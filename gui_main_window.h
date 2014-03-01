/// @file
///
/// @author Ralph Tandetzky
/// @date 28 Feb 2014

#pragma once

#include <QMainWindow>
#include <memory>

namespace gui
{

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow( QWidget * parent = nullptr );
    ~MainWindow();

private slots:
    void selectInputFile();
    void selectOutputFiles();
    void runConversion();
    
private:
    struct Impl;
    std::unique_ptr<Impl> m;
};

} // namespace gui
