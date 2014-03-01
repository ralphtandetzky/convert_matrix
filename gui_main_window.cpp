#include "gui_main_window.h"
#include "ui_gui_main_window.h"

#include "cpp_utils/exception.h"
#include "cpp_utils/extract_by_line.h"
#include "cpp_utils/more_algorithms.h"
#include "cpp_utils/std_make_unique.h"
#include "qt_utils/invoke_in_thread.h"
#include "qt_utils/loop_thread.h"
#include "qt_utils/serialize_props.h"

#include <QFileDialog>
#include <algorithm>
#include <fstream>
#include <functional>
#include <sstream>

namespace gui
{

struct MainWindow::Impl
{
    // Contains Qt user interface elements.
    Ui::MainWindow ui;
    // Objects which help to load the values in the gui input widgets
    // during construction and to store them during destruction.
    std::vector<std::unique_ptr<qu::PropertySerializer>> serializers;

    qu::LoopThread conversionThread;
};


MainWindow::MainWindow( QWidget * parent )
    : QMainWindow( parent )
{
    m = std::make_unique<Impl>();
    m->ui.setupUi(this);

    // set up serializers
    qu::createPropertySerializers( this->findChildren<QCheckBox*>(),
                                   std::back_inserter( m->serializers ) );
    qu::createPropertySerializers( this->findChildren<QLineEdit*>(),
                                   std::back_inserter( m->serializers ) );

    // load serialized input widget entries from a settings file.
    std::ifstream file( "settings.txt" );
    readProperties( file, m->serializers );
}


MainWindow::~MainWindow()
{
    // store current values from gui input widget entries.
    std::ofstream file( "settings.txt" );
    writeProperties( file, m->serializers );
}


void MainWindow::selectInputFile()
{
    const auto qFileName = QFileDialog::getOpenFileName(
                this, "Select Input File" );
    if ( qFileName.isNull() ) // user cancelled?
        return;
    m->ui.inputFileLineEdit->setText( qFileName );
}


void MainWindow::selectOutputFiles()
{
    const auto qFileName = QFileDialog::getSaveFileName(
                this, "Select Output File or File Pattern" );
    if ( qFileName.isNull() ) // user cancelled?
        return;
    m->ui.outputFilesLineEdit->setText( qFileName );
}


void MainWindow::runConversion()
{
    const auto inputFileName =
            m->ui.inputFileLineEdit->text().toStdString();
    const auto shallTranspose =
            m->ui.transposeCheckBox->isChecked();
    const auto shallCreateFileForEachRow =
            m->ui.fileForEachRowCheckBox->isChecked();
    const auto outputFileNames =
            m->ui.outputFilesLineEdit->text().toStdString();
    const auto replaceString =
            m->ui.replaceCharsLineEdit->text().toStdString();

    qu::invokeInThread( &m->conversionThread, [=]()
    {
        std::ifstream inputFile{ inputFileName };

        if ( !inputFile )
            CU_THROW( "Could not open the file '" + inputFileName + "\'." );

        const auto lines = cu::extractByLine( inputFile );

        if ( inputFile.bad() )
            CU_THROW( "The file '" + inputFileName +
                      "' could not be read." );
        if ( !inputFile.eof() )
            CU_THROW( "The end of the file '\"'" + inputFileName +
                      "' has not been reached." );

        // extract the values from each line
        std::vector<std::vector<double>> matrix;
        size_t nLine = 0;
        for ( const auto & line : lines )
        {
            ++nLine;
            std::istringstream is(line);
            matrix.push_back( {} );
            std::copy( std::istream_iterator<double>(is),
                       std::istream_iterator<double>(),
                       std::back_inserter( matrix.back() ) );
            if ( is.bad() || !is.eof() )
                CU_THROW( "Line " + std::to_string(nLine) +
                          " in file '" + inputFileName +
                          "' could not be parsed to the end." );
        }

        // remove empty rows
        matrix.erase( std::remove_if(
                begin(matrix), end(matrix),
                std::mem_fn(&std::vector<double>::empty) ),
            end(matrix) );

        if ( matrix.empty() )
            CU_THROW( "The file '" + inputFileName +
                      "' does not contain samples." );

        // Check if all rows have the same length
        nLine = 0;
        for ( const auto & row : matrix )
        {
            ++nLine;
            if ( row.size() != matrix.front().size() )
                CU_THROW( "Row " + std::to_string( nLine ) +
                          "of the matrix contains a different number of "
                          "samples than the first row." );
        }

        if ( shallTranspose )
        {
            std::vector<std::vector<double>> transposed( matrix.front().size() );

            for ( const auto & row : matrix )
                cu::for_each( begin(row), end(row),
                              begin(transposed), end(transposed),
                              []( double x, std::vector<double> & t )
                              { t.push_back(x); } );
            matrix.swap( transposed );
        }

        if ( shallCreateFileForEachRow )
        {
            if ( replaceString.empty() )
                CU_THROW( "No characters to be replaced in the output file "
                          "pattern have been specified." );
            const auto it = cu::findBoyerMoore(
                        begin(replaceString),
                        end(replaceString),
                        begin(outputFileNames),
                        end(outputFileNames) );
            if ( it == end(outputFileNames) )
                CU_THROW( "Replacement characters could not be found "
                          "in the output file pattern." );
            const auto outputFileNamesFirstPart = std::string(
                        begin(outputFileNames), it );
            const auto outputFileNamesLastPart = std::string(
                        it+replaceString.size(), end(outputFileNames) );
            nLine = 0;
            for ( const auto & row : matrix )
            {
                ++nLine;
                const auto outputFileName =
                        outputFileNamesFirstPart +
                        std::to_string(nLine) +
                        outputFileNamesLastPart;
                std::ofstream outputFile( outputFileName );
                std::copy( begin(row), end(row),
                           std::ostream_iterator<double>(outputFile, " ") );
                outputFile << std::endl;
                if ( !outputFile.good() )
                    CU_THROW( "Failed to write row " +
                              std::to_string(nLine) +
                              " to the file '" +
                              outputFileName + "'." );
            }
        }
        else // (!shallCreateFileForEachRow)
        {
            nLine = 0;
            std::ofstream outputFile( outputFileNames );
            for ( const auto & row : matrix )
            {
                ++nLine;
                std::copy( begin(row), end(row),
                           std::ostream_iterator<double>(outputFile, " ") );
                outputFile << std::endl;
                if ( !outputFile.good() )
                    CU_THROW( "Failed to write row " +
                              std::to_string(nLine) +
                              " to the file '" +
                              outputFileNames + "'." );
            }
        }
        qu::invokeInGuiThread( [this]
        {
            m->ui.statusBar->showMessage(
                   "Files written successfully.", 3000 );
        } );
    } );
}

} // namespace gui
