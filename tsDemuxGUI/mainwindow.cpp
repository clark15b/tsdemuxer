#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include "mpls.h"
#include "execwindow.h"

#ifdef _WIN32
#define os_slash    '\\'
#else
#define os_slash    '/'
#endif


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindowClass)
{
    ui->setupUi(this);

    connect(ui->actionOpen_AVCHD_Blu_Ray,SIGNAL(triggered()),this,SLOT(on_pushButton_clicked()));
    connect(ui->actionOpen_Directory,SIGNAL(triggered()),this,SLOT(on_pushButton_2_clicked()));
    connect(ui->actionOpen_File,SIGNAL(triggered()),this,SLOT(on_pushButton_3_clicked()));
    connect(ui->actionClear,SIGNAL(triggered()),this,SLOT(on_pushButton_4_clicked()));
    connect(ui->actionQuit,SIGNAL(triggered()),this,SLOT(close()));
    connect(ui->actionRun,SIGNAL(triggered()),this,SLOT(on_pushButton_7_clicked()));

    ui->lineEdit->setText(QString("1"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addRow(QTableWidget* w,const QStringList& l)
{
    int column=0;
    int row=w->rowCount();
    w->setRowCount(row+1);
    Q_FOREACH(QString s,l)
    {
        QStringList ll=s.split(';');

        if(ll.size()>0)
        {
            QTableWidgetItem* newItem=new QTableWidgetItem(ll[0]);
            if(ll.size()>1)
                newItem->setData(Qt::UserRole,ll[1]);
            w->setItem(row,column++,newItem);
        }
    }
}

void MainWindow::on_pushButton_clicked()
{
    std::string filename=QFileDialog::getOpenFileName(this,tr("MPLS source file"),last_dir.c_str(),"MPLS file (*.mpls *.mpl)").toLocal8Bit().data();

    if(!filename.length())
        return;

    std::string path;
    std::string stream_path;


    {
        std::string::size_type n=filename.find_last_of("\\/");

        if(n!=std::string::npos)
        {
            path=filename.substr(0,n);

            n=path.find_last_of("\\/");

            if(n!=std::string::npos)
                stream_path=path.substr(0,n);
        }
    }

    last_dir=path;


    if(stream_path.length())
    {
        QStringList filters;
        filters<<"STREAM";

        QDir dd(stream_path.c_str());
        QStringList l=dd.entryList(filters,QDir::Dirs);
        if(l.size()>0)
            stream_path=stream_path+os_slash+l[0].toLocal8Bit().data();
        else
            stream_path.clear();
    }

    std::list<int> playlist;
    std::map<int,std::string> datetime;

    int rc=mpls::parse(filename.c_str(),playlist,datetime,0);

    if(rc)
        return;

    ui->tableWidget->setRowCount(0);


    for(std::list<int>::iterator i=playlist.begin();i!=playlist.end();++i)
    {
        char buf[32]; sprintf(buf,"%.5i",*i);

        std::string fn;

        {
            QDir dd(stream_path.c_str());

            QStringList filters;
            filters<<QString("%1.mts").arg(buf)<<QString("%1.m2ts").arg(buf);

            QStringList l=dd.entryList(filters,QDir::Files);
            if(l.size()>0)
                fn=stream_path+os_slash+l[0].toLocal8Bit().data();
        }

        if(fn.length())
        {
            QStringList lst;

            std::string dn;

            std::string::size_type n=fn.find_last_of("\\/");
            if(n==std::string::npos)
                dn=fn;
            else
                dn=fn.substr(n+1);

            lst<<(dn+';'+fn).c_str()<<datetime[*i].c_str();

            addRow(ui->tableWidget,lst);

            ui->tableWidget->resizeColumnsToContents();
            ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
        }
    }
}


void MainWindow::on_pushButton_2_clicked()
{
    QString path=QFileDialog::getExistingDirectory(this,tr("source directory"),last_dir.c_str());

    if(path.isEmpty())
        return;

    last_dir=path.toLocal8Bit().data();

    QDir d(path);

    QStringList filters;
    filters<<"*.mts"<<"*.m2ts"<<"*.ts";

    QStringList l=d.entryList(filters,QDir::Files,QDir::Name);

    Q_FOREACH(QString s,l)
    {
        QStringList lst;
        lst<<s+';'+path+os_slash+s<<"";
        addRow(ui->tableWidget,lst);

        ui->tableWidget->resizeColumnsToContents();
        ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

    }
}

void MainWindow::on_pushButton_3_clicked()
{
    QStringList files=QFileDialog::getOpenFileNames(this,tr("source files"),last_dir.c_str(),"All files (*.m2ts *.mts *.ts);;M2TS files (*.m2ts *.mts);;TS files (*.ts)");

    if(files.size()>0)
    {
        std::string ss=files[0].toLocal8Bit().data();
        std::string::size_type n=ss.find_last_of("\\/");
        if(n!=std::string::npos)
            last_dir=ss.substr(0,n);
    }

    Q_FOREACH(QString s,files)
    {
        std::string ss=s.toLocal8Bit().data();
        std::string::size_type n=ss.find_last_of("\\/");
        if(n!=std::string::npos)
            ss=ss.substr(n+1);

        QStringList lst;
        lst<<(ss+';'+s.toLocal8Bit().data()).c_str()<<"";
        addRow(ui->tableWidget,lst);

        ui->tableWidget->resizeColumnsToContents();
        ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

    }

}

void MainWindow::on_pushButton_5_clicked()
{
    int row=ui->tableWidget->currentRow();

    if(row==-1)
        return;

    ui->tableWidget->removeRow(row);
}

void MainWindow::on_pushButton_4_clicked()
{
    ui->tableWidget->setRowCount(0);
}

void MainWindow::on_pushButton_6_clicked()
{
    QString path=QFileDialog::getExistingDirectory(this,tr("demux target directory"),"");

#ifdef _WIN32
    path=path.replace('/','\\');
#endif

    if(!path.isEmpty())
        ui->lineEdit_2->setText(path);
}

void MainWindow::on_checkBox_3_stateChanged(int st)
{
    switch(st)
    {
    case Qt::Checked:
        ui->checkBox_2->setCheckState(Qt::Unchecked);
        ui->checkBox_2->setEnabled(false);
        break;
    case Qt::Unchecked:
        ui->checkBox_2->setEnabled(true);
        break;
    }
}

void MainWindow::on_pushButton_8_clicked()
{
    int row=ui->tableWidget->currentRow();

    if(row<1)
        return;

    ui->tableWidget->insertRow(row-1);
    ui->tableWidget->setItem(row-1,0,ui->tableWidget->takeItem(row+1,0));
    ui->tableWidget->setItem(row-1,1,ui->tableWidget->takeItem(row+1,1));
    ui->tableWidget->removeRow(row+1);

    ui->tableWidget->setCurrentCell(row-1,0);
}


void MainWindow::on_pushButton_9_clicked()
{
    int row=ui->tableWidget->currentRow();

    if(row>=ui->tableWidget->rowCount()-1)
        return;

    ui->tableWidget->insertRow(row+2);
    ui->tableWidget->setItem(row+2,0,ui->tableWidget->takeItem(row,0));
    ui->tableWidget->setItem(row+2,1,ui->tableWidget->takeItem(row,1));
    ui->tableWidget->removeRow(row);

    ui->tableWidget->setCurrentCell(row+1,0);

}


void MainWindow::on_pushButton_7_clicked()
{
    if(ui->tableWidget->rowCount()<1)
        return;

    std::string dst=ui->lineEdit_2->text().toLocal8Bit().data();

    if(dst.length())
        dst+=os_slash;

    std::string playlist=dst+"playlist.txt";

    FILE* fp=fopen(playlist.c_str(),"w");

    if(!fp)
        return;

    for(int i=0;i<ui->tableWidget->rowCount();i++)
    {
        QTableWidgetItem* item=ui->tableWidget->item(i,0);
        QString desc=ui->tableWidget->item(i,1)->text();

        if(desc.isEmpty())
            fprintf(fp,"%s\n",item->data(Qt::UserRole).toString().toLocal8Bit().data());
        else
            fprintf(fp,"%s;%s\n",item->data(Qt::UserRole).toString().toLocal8Bit().data(),desc.toLocal8Bit().data());
    }

    fclose(fp);

    std::string channel=ui->lineEdit->text().toLocal8Bit().data();
    bool demux_all_stream=ui->checkBox->checkState()==Qt::Checked?true:false;
    bool join_stream=ui->checkBox_2->checkState()==Qt::Checked?true:false;
    bool pes_stream=ui->checkBox_3->checkState()==Qt::Checked?true:false;

    QStringList lst;

    lst<<"-s"<<playlist.c_str();
    if(dst.length())
        lst<<"-o"<<dst.c_str();
    if(channel.length())
        lst<<"-c"<<channel.c_str();
    if(demux_all_stream)
        lst<<"-u";
    if(join_stream)
        lst<<"-j";
    if(pes_stream)
        lst<<"-z";
    lst<<"-m";

    execWindow dlg(this,tr("<font color=gray>tsDemuxerGUI<br>Copyright (C) 2009 Anton Burdinuk<br>clark15b@gmail.com<br>http://code.google.com/p/tsdemuxer<br>---------------------------------------------------<br><br></font>"));

    dlg.batch.push_back(execCmd(tr("Demuxing"),
            tr("Demuxing..."),
            "./tsdemux",lst));

    dlg.run();

    remove(playlist.c_str());
}

