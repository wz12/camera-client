#include "widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{

    ui->setupUi(this);
    InitGUI();
}

Widget::~Widget()
{
    freeList();

    if(ftpPreView!=NULL)
    {
        delete ftpPreView;
        ftpPreView=NULL;
    }
    if(m_hLocalAlgDebug!=NULL)
    {
        delete m_hLocalAlgDebug;
        m_hLocalAlgDebug=NULL;
    }
    delete ui;
}
void Widget::InitGUI()
{
    this->setMinimumSize(1280,720);
    setWindowFlags(Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground,true);

    SetupAttachedWindow();
    //  setWindowOpacity(0.5);
    SetupLog();

    SerialPortWidget=new SerialPort();
    config = new ConfigFile;

    connect(config,SIGNAL(sendToLog(QString)),this,SLOT(LogDebug(QString)));
    //    connect(ui->button_close,SIGNAL(clicked()),SerialPortWidget,SLOT(close()));
    connect(ui->button_close,SIGNAL(clicked()),this,SLOT(close()));
    connect(ui->button_SerialPort,SIGNAL(clicked()),this,SLOT(OpenSubWindow_SerialPort()));
    connect(this,SIGNAL(SubWindow_Init()),SerialPortWidget,SLOT(SerialPort_Init()));

    SetupNetwork();
    SetupList();
    SetupMouseEvent();
    SetupVideo();
    SetupCalibrate();
    SetupRun();
    SetupConnection();
    SetupOptions();
    SetupInformations();
    SetupAccount();

    ui->mainlist->setVisible(false);
    ui->sublist->setCurrentIndex(ui->sublist->count()-1);
    ui->button_Save->setEnabled(false);
    ui->button_SaveAs->setEnabled(false);

#ifdef OFFLINE_DEBUG
    ui->sublist->setCurrentIndex(4);
    ui->mainlist->setVisible(true);
    config->initAlgService();
    //    config->initAlgService();
    //    qDebug()<<config->getAlgResultSize();
#endif

}

void Widget::SetupAttachedWindow()
{
    SaveasMenu = new QMenu;
    SaveasMenu->setObjectName("saveasmenu");
    QWidget* SaveasSubwidget = new QWidget();
    SaveasSubwidget->setObjectName("saveassubwidget");
    SaveasSubwidget->setStyleSheet("#saveassubwidget{background-color:rgba(50,50,50,220);}"
                                   "QLabel{color:white;font:bold 13pt Arial;}"
                                   "QPushButton{font:bold 13pt Arial;}");
    SaveasSubwidget->setFixedSize(200,100);
    QVBoxLayout *SaveasLayout = new QVBoxLayout;
    QLabel *SaveasLabel = new QLabel("Config File:");
    SaveasComboBox = new QComboBox;
    SaveasComboBox->setEditable(true);
    QPushButton *SaveasButton = new QPushButton("OK");
    connect(SaveasButton,SIGNAL(clicked()),this,SLOT(ConfigSaveAs()));
    SaveasLayout->addWidget(SaveasLabel);
    SaveasLayout->addWidget(SaveasComboBox);
    SaveasLayout->addWidget(SaveasButton);
    SaveasSubwidget->setLayout(SaveasLayout);
    QWidgetAction* SaveasAction=new QWidgetAction(SaveasMenu);
    SaveasAction->setDefaultWidget(SaveasSubwidget);
    SaveasMenu->addAction(SaveasAction);
    SaveasMenu->setWindowFlags(Qt::FramelessWindowHint|Qt::Popup);
    SaveasMenu->setAttribute(Qt::WA_TranslucentBackground);
    SaveasMenu->setStyleSheet("#saveasmenu{border-width:0px;background-color:rgba(0,0,0,0);}");
}
void Widget::OpenSubWindow_SerialPort()
{

    if(SerialPortWidget!=NULL)
        SerialPortWidget->show();
    emit SubWindow_Init();

}

void Widget::SetupMouseEvent()
{
    isLeftPressed=false;
    curPos=0;
    QCursor cursor;
    cursor.setShape(Qt::ArrowCursor);
    QWidget::setCursor(cursor);
    this->setMouseTracking(true);
    ui->mainframe->setMouseTracking(true);
}

void Widget::SetupOptions()
{
    QDir updatedir;
    if(updatedir.mkdir("./update/"))
    {
        LogDebug("did not find update dir, create update dir.");
        qDebug()<<"did not find update dir, create update dir.";
    }
    else
    {
        LogDebug("find update dir.");
        qDebug()<<"find update dir.";
    }

    updateWidget = new FirmwareUpdate(this);
    updateWidget->setWindowFlags(Qt::Popup);
    connect(updateWidget,SIGNAL(updateOption(int)),this,SLOT(firmwareUpdateService(int)));

    /* for config list browser*/
    ui->system_configlist->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->system_configlist->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->system_configlist->setEditTriggers(QAbstractItemView::NoEditTriggers);
    //    ui->system_configlist->setAlternatingRowColors(true);
    ui->system_configlist->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->system_configlist->setColumnCount(5);
    QStringList sys_header;
    sys_header<<"Name"<<"Creator"<<"ChangedBy"<<"Loaded"<<" ";
    ui->system_configlist->setHorizontalHeaderLabels(sys_header);
    ui->system_configlist->verticalHeader()->setVisible(false);
    //    ui->system_configlist->horizontalHeader()->setStyleSheet("QHeaderView::section{background-color:rgba(0,0,0,0);}");

    //ui->system_configlist->setStyleSheet("background-color:rgba(50,50,50,120);");

    /* for user account browser */
    connect(ui->security_addusercontrolokButton,SIGNAL(clicked()),this,SLOT(UserAdd()));

    ui->security_tablewidget->setSelectionMode(QAbstractItemView::NoSelection);
    ui->security_tablewidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->security_tablewidget->setAlternatingRowColors(true);
    ui->security_tablewidget->setColumnCount(3);
    QStringList security_Hheader;
    security_Hheader<<"User Name"<<"Authority"<<"";
    ui->security_tablewidget->setHorizontalHeaderLabels(security_Hheader);
    ui->security_tablewidget->horizontalHeader()->setSectionsClickable(false);
    ui->security_tablewidget->verticalHeader()->setVisible(false);
    ui->security_tablewidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    //    ui->security_tablewidget->setStyleSheet("background-color:rgba(50,50,50,120);");
    //    for(i=0;i<ui->security_tablewidget->rowCount();i++)
    //        ui->security_tablewidget->horizontalHeaderItem(i)->setTextAlignment(Qt::AlignCenter);

    /*for local alg debug*/
    m_hLocalAlgDebug = new algDebug;
    m_hLocalAlgDebug->setDisplayWidget(ui->diagnostic_previewWidget);
    connect(m_hLocalAlgDebug,&algDebug::sendFrameToServer,this,[=](QByteArray ba){
        QString targetPath = ALG_DBGIMG;
        emit network->ftp_put(targetPath,ba);
    });
    connect(ui->diagnostic_local_loadButton_2,&QPushButton::clicked,this,[=](){
        network->sendConfigToServer(NET_MSG_IMGALG_UPDATE_DBGIMG,1);
    });


    /*for ftp browser*/
    connect(ui->diagnostic_refreshButton,&QPushButton::clicked,network,&NetWork::ftp_refresh);
    connect(network,&NetWork::ftp_receiveFileList,this,[=](QByteArray ba){
        UpdateFtpList(ba);
    });

    connect(network,&NetWork::ftp_receiveData,this,&Widget::saveFtpData);
    //    connect(ui->diagnostic_deleteButton,&QPushButton::clicked,this,[=](){
    //        emit network->ftp_del(ui->diagnostic_itemnamecurrentLabel->text());
    //    });
    ui->diagnostic_ftpbrowsertablewidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->diagnostic_ftpbrowsertablewidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->diagnostic_ftpbrowsertablewidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->diagnostic_ftpbrowsertablewidget->setAlternatingRowColors(true);
    ui->diagnostic_ftpbrowsertablewidget->setColumnCount(3);
    QStringList diagnostic_header;
    diagnostic_header<<"Name"<<"Size"<<"Type";
    ui->diagnostic_ftpbrowsertablewidget->setHorizontalHeaderLabels(diagnostic_header);
    ui->diagnostic_ftpbrowsertablewidget->horizontalHeader()->setSectionsClickable(false);
    ui->diagnostic_ftpbrowsertablewidget->verticalHeader()->setVisible(false);
    ui->diagnostic_ftpbrowsertablewidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    /*first update config list*/
    UpdateConfigFile();

}

void Widget::SetupCalibrate()
{
    ui->light_led1pwmSlider->setRange(0,100);
    ui->light_led2pwmSlider->setRange(0,100);

    ui->camera_contrastSlider->setRange(0,255);
    ui->camera_saturationSlider->setRange(0,255);
    ui->camera_sharpnessSlider->setRange(0,255);
    ui->camera_brightnessSlider->setRange(0,255);
    ui->camera_exposureSlider->setRange(0,255);
    ui->camera_sensorgainSlider->setRange(0,255);
    ui->camera_pipegainSlider->setRange(0,255);
    ui->video_JPEGqualitySlider->setRange(0,100);

    connect(ui->light_led1pwmSlider,SIGNAL(sliderMoved(int)),ui->light_led1pwmlineEdit,SLOT(setValue(int)));
    connect(ui->light_led1pwmSlider,SIGNAL(sliderReleased()),ui->light_led1pwmlineEdit,SLOT(sendValueSignal()));
    connect(ui->light_led1pwmlineEdit,SIGNAL(valueFinished(int)),ui->light_led1pwmSlider,SLOT(setValue(int)));
    connect(ui->light_led1pwmlineEdit,SIGNAL(valueUpdateStr(int)),config,SLOT(setLightConfig(int)));
    connect(ui->light_led2pwmSlider,SIGNAL(sliderMoved(int)),ui->light_led2pwmlineEdit,SLOT(setValue(int)));
    connect(ui->light_led2pwmSlider,SIGNAL(sliderReleased()),ui->light_led2pwmlineEdit,SLOT(sendValueSignal()));
    connect(ui->light_led2pwmlineEdit,SIGNAL(valueFinished(int)),ui->light_led2pwmSlider,SLOT(setValue(int)));
    connect(ui->light_led2pwmlineEdit,SIGNAL(valueUpdateStr(int)),config,SLOT(setLightConfig(int)));
    connect(ui->light_led1enablecomboBox,SIGNAL(activated(int)),config,SLOT(setLightConfig(int)));
    connect(ui->light_led2enablecomboBox,SIGNAL(activated(int)),config,SLOT(setLightConfig(int)));
    connect(config,SIGNAL(sendToServerLightConfig(QVariant)),network,SLOT(sendConfigToServerLightConfig(QVariant)));

    //Camera_Normal immediate updating
    connect(ui->camera_contrastSlider,SIGNAL(sliderMoved(int)),ui->camera_contrastlineEdit,SLOT(setValue(int)));
    connect(ui->camera_contrastSlider,SIGNAL(sliderReleased()),ui->camera_contrastlineEdit,SLOT(sendValueSignal()));
    connect(ui->camera_contrastlineEdit,SIGNAL(valueFinished(int)),ui->camera_contrastSlider,SLOT(setValue(int)));
    connect(ui->camera_contrastlineEdit,SIGNAL(valueUpdateStr(int)),config,SLOT(setContrast(int)));
    connect(ui->camera_saturationSlider,SIGNAL(sliderMoved(int)),ui->camera_saturationlineEdit,SLOT(setValue(int)));
    connect(ui->camera_saturationSlider,SIGNAL(sliderReleased()),ui->camera_saturationlineEdit,SLOT(sendValueSignal()));
    connect(ui->camera_saturationlineEdit,SIGNAL(valueFinished(int)),ui->camera_saturationSlider,SLOT(setValue(int)));
    connect(ui->camera_saturationlineEdit,SIGNAL(valueUpdateStr(int)),config,SLOT(setSaturation(int)));
    connect(ui->camera_sharpnessSlider,SIGNAL(sliderMoved(int)),ui->camera_sharpnesslineEdit,SLOT(setValue(int)));
    connect(ui->camera_sharpnessSlider,SIGNAL(sliderReleased()),ui->camera_sharpnesslineEdit,SLOT(sendValueSignal()));
    connect(ui->camera_sharpnesslineEdit,SIGNAL(valueFinished(int)),ui->camera_sharpnessSlider,SLOT(setValue(int)));
    connect(ui->camera_sharpnesslineEdit,SIGNAL(valueUpdateStr(int)),config,SLOT(setSharpness(int)));
    connect(ui->camera_exposureSlider,SIGNAL(sliderMoved(int)),ui->camera_exposurelineEdit,SLOT(setValue(int)));
    connect(ui->camera_exposureSlider,SIGNAL(sliderReleased()),ui->camera_exposurelineEdit,SLOT(sendValueSignal()));
    connect(ui->camera_exposurelineEdit,SIGNAL(valueFinished(int)),ui->camera_exposureSlider,SLOT(setValue(int)));
    connect(ui->camera_exposurelineEdit,SIGNAL(valueUpdateStr(int)),config,SLOT(setExposure(int)));
    connect(ui->camera_sensorgainSlider,SIGNAL(sliderMoved(int)),ui->camera_sensorgainlineEdit,SLOT(setValue(int)));
    connect(ui->camera_sensorgainSlider,SIGNAL(sliderReleased()),ui->camera_sensorgainlineEdit,SLOT(sendValueSignal()));
    connect(ui->camera_sensorgainlineEdit,SIGNAL(valueFinished(int)),ui->camera_sensorgainSlider,SLOT(setValue(int)));
    connect(ui->camera_sensorgainlineEdit,SIGNAL(valueUpdateStr(int)),config,SLOT(setSensorGain(int)));
    connect(ui->camera_pipegainSlider,SIGNAL(sliderMoved(int)),ui->camera_pipegainlineEdit,SLOT(setValue(int)));
    connect(ui->camera_pipegainSlider,SIGNAL(sliderReleased()),ui->camera_pipegainlineEdit,SLOT(sendValueSignal()));
    connect(ui->camera_pipegainlineEdit,SIGNAL(valueFinished(int)),ui->camera_pipegainSlider,SLOT(setValue(int)));
    connect(ui->camera_pipegainlineEdit,SIGNAL(valueUpdateStr(int)),config,SLOT(setPipeGain(int)));
    connect(ui->camera_brightnessSlider,SIGNAL(sliderMoved(int)),ui->camera_brightnesslineEdit,SLOT(setValue(int)));
    connect(ui->camera_brightnessSlider,SIGNAL(sliderReleased()),ui->camera_brightnesslineEdit,SLOT(sendValueSignal()));
    connect(ui->camera_brightnesslineEdit,SIGNAL(valueFinished(int)),ui->camera_brightnessSlider,SLOT(setValue(int)));
    connect(ui->camera_brightnesslineEdit,SIGNAL(valueUpdateStr(int)),config,SLOT(setBrightness(int)));
    connect(ui->camera_2AmodecomboBox,SIGNAL(activated(int)),config,SLOT(set2AMode(int)));
    connect(ui->camera_2AmodecomboBox,SIGNAL(activated(int)),this,SLOT(camera_2AmodecomboBox_Service(int)));
    connect(ui->camera_whitebalancemodecomboBox,SIGNAL(activated(int)),config,SLOT(setWhiteBalanceMode(int)));
    connect(ui->camera_daynightmodecomboBox,SIGNAL(activated(int)),config,SLOT(setDayNightMode(int)));
    connect(ui->camera_binningcomboBox,SIGNAL(activated(int)),config,SLOT(setBinning(int)));
    connect(ui->camera_mirrorcomboBox,SIGNAL(activated(int)),config,SLOT(setMirror(int)));
    connect(ui->camera_expprioritycomboBox,SIGNAL(activated(int)),config,SLOT(setExpPriority(int)));

    //Camera_2A entire updating
    connect(ui->camera_2A_TargetBrightnessSlider,SIGNAL(sliderMoved(int)),ui->camera_2A_TargetBrightnesslineEdit,SLOT(setValue(int)));
    connect(ui->camera_2A_TargetBrightnessSlider,SIGNAL(sliderReleased()),ui->camera_2A_TargetBrightnesslineEdit,SLOT(sendValueSignal()));
    connect(ui->camera_2A_TargetBrightnesslineEdit,SIGNAL(valueFinished(int)),ui->camera_2A_TargetBrightnessSlider,SLOT(setValue(int)));
    connect(ui->camera_2A_MaxBrightnessSlider,SIGNAL(sliderMoved(int)),ui->camera_2A_MaxBrightnesslineEdit,SLOT(setValue(int)));
    connect(ui->camera_2A_MaxBrightnessSlider,SIGNAL(sliderReleased()),ui->camera_2A_MaxBrightnesslineEdit,SLOT(sendValueSignal()));
    connect(ui->camera_2A_MaxBrightnesslineEdit,SIGNAL(valueFinished(int)),ui->camera_2A_MaxBrightnessSlider,SLOT(setValue(int)));
    connect(ui->camera_2A_MinBrightnessSlider,SIGNAL(sliderMoved(int)),ui->camera_2A_MinBrightnesslineEdit,SLOT(setValue(int)));
    connect(ui->camera_2A_MinBrightnessSlider,SIGNAL(sliderReleased()),ui->camera_2A_MinBrightnesslineEdit,SLOT(sendValueSignal()));
    connect(ui->camera_2A_MinBrightnesslineEdit,SIGNAL(valueFinished(int)),ui->camera_2A_MinBrightnessSlider,SLOT(setValue(int)));
    connect(ui->camera_2A_ThresholdSlider,SIGNAL(sliderMoved(int)),ui->camera_2A_ThresholdlineEdit,SLOT(setValue(int)));
    connect(ui->camera_2A_ThresholdSlider,SIGNAL(sliderReleased()),ui->camera_2A_ThresholdlineEdit,SLOT(sendValueSignal()));
    connect(ui->camera_2A_ThresholdlineEdit,SIGNAL(valueFinished(int)),ui->camera_2A_ThresholdSlider,SLOT(setValue(int)));

    //Camera_advanced immediate updating
    connect(ui->camera_vsenablecomboBox,SIGNAL(activated(int)),config,SLOT(setVsEnable(int)));
    connect(ui->camera_ldcenablecomboBox,SIGNAL(activated(int)),config,SLOT(setLDCEnable(int)));
    connect(ui->camera_vnfenablecomboBox,SIGNAL(activated(int)),config,SLOT(setVnfEnable(int)));
    connect(ui->camera_vnfmodecomboBox,SIGNAL(activated(int)),config,SLOT(setVnfMode(int)));

    //Video immediate updating
    QStackedLayout* videoLayout = new QStackedLayout;
    camera_videoinputwidget = new MultiImageWidget;
    camera_videoinputwidget->resizeImageCount(1);
    ui->camera_videoWidget->setLayout(videoLayout);

    //videoLayout->addWidget(ui->camera_2A_AEWeight_preWidget);
    ui->camera_2A_AEWeight_preWidget->hide();
    videoLayout->addWidget(camera_videoinputwidget);
    videoLayout->setStackingMode(QStackedLayout::StackAll);

    //videoLayout->addWidget()
    connect(ui->video_streamtypecomboBox,SIGNAL(activated(int)),config,SLOT(setStreamType(int)));
    connect(ui->video_videocodecmodecomboBox,SIGNAL(activated(int)),config,SLOT(setVideoCodecMode(int)));
    connect(ui->video_JPEGqualitySlider,SIGNAL(sliderMoved(int)),ui->video_JPEGqualitylineEdit,SLOT(setValue(int)));
    connect(ui->video_JPEGqualitySlider,SIGNAL(sliderReleased()),ui->video_JPEGqualitylineEdit,SLOT(sendValueSignal()));
    connect(ui->video_JPEGqualitylineEdit,SIGNAL(valueFinished(int)),ui->video_JPEGqualitySlider,SLOT(setValue(int)));
    connect(ui->video_JPEGqualitylineEdit,SIGNAL(valueUpdateStr(int)),config,SLOT(setJPEGQuality(int)));

    //Algorithm
    //    connect(this,SIGNAL(setALGParams()),config,SLOT(setAlgParams()));
    connect(config,SIGNAL(algConfigTag(QVBoxLayout*,QPoint)),this,SLOT(loadALGConfigUi(QVBoxLayout*,QPoint)));
    connect(config,SIGNAL(algResultTag(QVBoxLayout*,QPoint)),this,SLOT(loadALGResultUi(QVBoxLayout*,QPoint)));
    //connect(ui->camera_2A_AEWeight_preWidget,SIGNAL(sendWeightInfo(EzCamH3AWeight)),this,SLOT(getH3AWeight(EzCamH3AWeight)));
}

void Widget::UpdateConfigFile()
{
    LogDebug("update the cfg file dir");
    qDebug()<<"update the cfg file dir";
    QString cfgname = config->getCurrentConfigName();
    int i=0;
    QFileInfoList infolist =QDir(config->getConfigDir()).entryInfoList();
    ui->system_configlist->clearContents();
    ui->system_configlist->setRowCount(0);
    ui->run_processmodeConfigBox->clear();
    SaveasComboBox->clear();
    foreach(QFileInfo fileinfo,infolist)
    {
        if(!fileinfo.isFile())continue;
        if(fileinfo.suffix().compare("ini")==0)
        {
            config->OpenConfig(fileinfo.baseName());

            ui->run_processmodeConfigBox->insertItem(ui->run_processmodeConfigBox->count(),fileinfo.baseName());
            SaveasComboBox->insertItem(SaveasComboBox->count(),fileinfo.baseName());
            ui->system_configlist->insertRow(ui->system_configlist->rowCount());
            ui->system_configlist->setItem(i,0,new QTableWidgetItem(fileinfo.baseName()));
            ui->system_configlist->setItem(i,1,new QTableWidgetItem(config->ConfigCreator()));
            ui->system_configlist->setItem(i,2,new QTableWidgetItem(config->ConfigChangedby()));

            if(cfgname==fileinfo.baseName())
            {
                ui->system_configlist->setItem(i,3,new QTableWidgetItem("true"));
                ui->run_currentconfigLabel->setText("Current Config: "+fileinfo.baseName());
            }
            else
                ui->system_configlist->setItem(i,3,new QTableWidgetItem("false"));
            QPushButton *pbutton = new QPushButton("delete");
            connect(pbutton,SIGNAL(clicked()),this,SLOT(ConfigDelete()));
            ui->system_configlist->setCellWidget(i,4,pbutton);
            i++;
        }
    }
    if(cfgname=="")
    {
        LogDebug("first time update cfg file dir");
        qDebug()<<"first time update cfg file dir";
    }
    else
    {
        LogDebug("restore the current cfg file"+cfgname);
        qDebug()<<"restore the current cfg file"+cfgname;
        config->OpenConfig(cfgname);
        ui->run_processmodeConfigBox->setCurrentText(config->getCurrentConfigName());
    }
}

void Widget::UpdateUserAccountList()
{

    ui->security_tablewidget->clearContents();
    ui->security_tablewidget->setRowCount(0);
    get_userdata_t userlist;
    if(!network->GetUser(&userlist))
    {
        emit LogDebug("GetUserlist failed.");
        qDebug()<<"GetUserlist failed.";
        if(ui->Accountlist->currentIndex()==0)
            recoverLoginState();
        return;
    }
    for(int i=0;i<userlist.userCount;i++)
    {
        qDebug()<<"find user: "<<userlist.user[i].user;
        ui->security_tablewidget->insertRow(ui->security_tablewidget->rowCount());
        QTableWidgetItem* user = new QTableWidgetItem(userlist.user[i].user);
        user->setTextAlignment(Qt::AlignCenter);
        ui->security_tablewidget->setItem(i,0,user);
        QTableWidgetItem* authority = new QTableWidgetItem;
        if(userlist.user[i].authority==AUTHORITY_ADMIN)
            authority->setText("Admin");
        else
            authority->setText("Viewer");
        authority->setTextAlignment(Qt::AlignCenter);
        ui->security_tablewidget->setItem(i,1,authority);
        if(user->text()!="admin")
        {
            QPushButton *delbutton = new QPushButton("delete");
            connect(delbutton,SIGNAL(clicked()),this,SLOT(deleteUser()));
            ui->security_tablewidget->setCellWidget(i,2,delbutton);
        }

    }
}

void Widget::UpdateCameraList(int id, QString IP)
{
    QString cameraInfo = QString::number(id,10)+"\t"+IP;
    ui->Account_cameralist->addItem(cameraInfo);
}

void Widget::UpdateFtpList(QByteArray ba)
{
    FileList* list = (FileList*)malloc(ba.length());
    memcpy(list,ba.data(),ba.length());

    ui->diagnostic_ftpbrowsertablewidget->insertRow(ui->diagnostic_ftpbrowsertablewidget->rowCount());
    ui->diagnostic_ftpbrowsertablewidget->setItem(ui->diagnostic_ftpbrowsertablewidget->rowCount()-1,0,new QTableWidgetItem("..."));
    unsigned int i;
    for(i=0;i<list->dirCount;i++)
    {
        ui->diagnostic_ftpbrowsertablewidget->insertRow(ui->diagnostic_ftpbrowsertablewidget->rowCount());
        ui->diagnostic_ftpbrowsertablewidget->setItem(ui->diagnostic_ftpbrowsertablewidget->rowCount()-1,0,new QTableWidgetItem(QString(list->finfo[i].name)));
        ui->diagnostic_ftpbrowsertablewidget->setItem(ui->diagnostic_ftpbrowsertablewidget->rowCount()-1,1,new QTableWidgetItem(QString::number(list->finfo[i].size,10)));
        ui->diagnostic_ftpbrowsertablewidget->setItem(ui->diagnostic_ftpbrowsertablewidget->rowCount()-1,2,new QTableWidgetItem(ftp_service::getType(list->finfo[i].type)));
    }
    free(list);
}

void Widget::saveFtpData(QByteArray data)//20170119, 解�??�?�???�?
{
    EzImgFrameHeader header;
    memcpy(&header,data.data(),sizeof(EzImgFrameHeader));
    int headsize = sizeof(EzImgFrameHeader)+header.infoSize;
    //qDebug()<<header.height<<header.width<<header.pitch<<header.infoSize;
    if(ftpPreView!=NULL)
    {
        delete ftpPreView;
        ftpPreView=NULL;
    }
    ftpPreView = new QImage((unsigned char*)data.data()+headsize,1280,720,header.pitch,QImage::Format_Grayscale8);
    *ftpPreView=ftpPreView->copy();
    ui->diagnostic_previewWidget->setImage(ftpPreView->scaledToWidth(ui->diagnostic_previewWidget->width()),0);
}

void Widget::LoadFullConfig()
{
    LoadRunConfig();
    LoadCalibrateConfig();
    LoadConnectionConfig();
    LoadInfomationConfig();
    LoadAccountConfig();
}

void Widget::LoadCalibrateConfig()
{
    CalibrateStr tempconfig = config->getCalibrate();
    ui->light_led1enablecomboBox->setCurrentIndex(tempconfig.value.light_config[0].enable);
    ui->light_led2enablecomboBox->setCurrentIndex(tempconfig.value.light_config[1].enable);
    ui->light_led1pwmlineEdit->editValue(tempconfig.value.light_config[0].pwmduty);
    ui->light_led2pwmlineEdit->editValue(tempconfig.value.light_config[1].pwmduty);
    ui->camera_contrastlineEdit->editValue(tempconfig.value.camera_Contrast);
    ui->camera_saturationlineEdit->editValue(tempconfig.value.camera_Saturation);
    ui->camera_sharpnesslineEdit->editValue(tempconfig.value.camera_Sharpness);
    ui->camera_exposurelineEdit->editValue(tempconfig.value.camera_Exposure);
    ui->camera_sensorgainlineEdit->editValue(tempconfig.value.camera_SensorGain);
    ui->camera_pipegainlineEdit->editValue(tempconfig.value.camera_PipeGain);
    ui->camera_brightnesslineEdit->editValue(tempconfig.value.camera_Brightness);
    ui->camera_whitebalancemodecomboBox->setCurrentIndex(tempconfig.value.camera_WhiteBalanceMode);
    ui->camera_daynightmodecomboBox->setCurrentIndex(tempconfig.value.camera_DayNightMode);
    ui->camera_binningcomboBox->setCurrentIndex(tempconfig.value.camera_Binning);
    ui->camera_mirrorcomboBox->setCurrentIndex(tempconfig.value.camera_Mirror);
    ui->camera_expprioritycomboBox->setCurrentIndex(tempconfig.value.camera_ExpPriority);
    ui->camera_2AmodecomboBox->setCurrentIndex(tempconfig.value.camera_2AMode);
    ui->camera_vsenablecomboBox->setCurrentIndex(tempconfig.value.camera_VsEnable);
    ui->camera_ldcenablecomboBox->setCurrentIndex(tempconfig.value.camera_LdcEnable);
    ui->camera_vnfenablecomboBox->setCurrentIndex(tempconfig.value.camera_VnfEnable);
    ui->camera_vnfmodecomboBox->setCurrentIndex(tempconfig.value.camera_VnfMode);

    if(ui->camera_2AmodecomboBox->currentIndex()==0||ui->camera_2AmodecomboBox->currentIndex()==2)
    {
        ui->camera_exposurelineEdit->setEnabled(true);
        ui->camera_exposureSlider->setEnabled(true);
        ui->camera_pipegainlineEdit->setEnabled(true);
        ui->camera_pipegainSlider->setEnabled(true);
        ui->camera_sensorgainlineEdit->setEnabled(true);
        ui->camera_sensorgainSlider->setEnabled(true);
    }
    else
    {
        ui->camera_exposurelineEdit->setEnabled(false);
        ui->camera_exposureSlider->setEnabled(false);
        ui->camera_pipegainlineEdit->setEnabled(false);
        ui->camera_pipegainSlider->setEnabled(false);
        ui->camera_sensorgainlineEdit->setEnabled(false);
        ui->camera_sensorgainSlider->setEnabled(false);
    }

    ui->camera_2A_TargetBrightnesslineEdit->editValue(tempconfig.value.camera_2A_TargetBrightness);
    ui->camera_2A_MaxBrightnesslineEdit->editValue(tempconfig.value.camera_2A_MaxBrightness);
    ui->camera_2A_MinBrightnesslineEdit->editValue(tempconfig.value.camera_2A_MinBrightness);
    ui->camera_2A_ThresholdlineEdit->editValue(tempconfig.value.camera_2A_Threshold);
    ui->camera_2A_widthValueLabel->setText(QString::number(tempconfig.value.camera_2A_width,10));
    ui->camera_2A_heightValueLabel->setText(QString::number(tempconfig.value.camera_2A_height,10));
    ui->camera_2A_HCountValueLabel->setText(QString::number(tempconfig.value.camera_2A_HCount,10));
    ui->camera_2A_VCountValueLabel->setText(QString::number(tempconfig.value.camera_2A_VCount,10));
    ui->camera_2A_HStartValueLabel->setText(QString::number(tempconfig.value.camera_2A_HStart,10));
    ui->camera_2A_VStartValueLabel->setText(QString::number(tempconfig.value.camera_2A_VStart,10));
    ui->camera_2A_horzIncrValueLabel->setText(QString::number(tempconfig.value.camera_2A_horzIncr,10));
    ui->camera_2A_vertIncrValueLabel->setText(QString::number(tempconfig.value.camera_2A_vertIncr,10));
    ui->camera_2A_AEWeight_width1lineEdit->setText(QString::number(tempconfig.value.camera_2A_weight_width1));
    ui->camera_2A_AEWeight_height1lineEdit->setText(QString::number(tempconfig.value.camera_2A_weight_height1));
    ui->camera_2A_AEWeight_h_start2lineEdit->setText(QString::number(tempconfig.value.camera_2A_weight_h_start2));
    ui->camera_2A_AEWeight_v_start2lineEdit->setText(QString::number(tempconfig.value.camera_2A_weight_v_start2));
    ui->camera_2A_AEWeight_width2lineEdit->setText(QString::number(tempconfig.value.camera_2A_weight_width2));
    ui->camera_2A_AEWeight_height2lineEdit->setText(QString::number(tempconfig.value.camera_2A_weight_height2));
    ui->camera_2A_AEWeight_weightlineEdit->setText(QString::number(tempconfig.value.camera_2A_weight_weight));
    //ui->camera_2A_AEWeight_preWidget->weightResize(tempconfig.value.camera_2A_HCount,tempconfig.value.camera_2A_VCount);

    ui->video_streamtypecomboBox->setCurrentIndex(tempconfig.value.camera_video_StreamType);
    ui->video_videocodecmodecomboBox->setCurrentIndex(tempconfig.value.camera_video_VideoCodecMode);
    ui->video_JPEGqualitylineEdit->editValue(tempconfig.value.camera_video_JPEGQuality);
}

void Widget::LoadRunConfig()
{
    //    RunStr tempconfig = config->getRun();
    //    ui->run_processmodeAlgBox->setChecked(tempconfig.value.LoadAlg);
    ServerState tempstate = config->getState();
    ui->run_saveResultBox->setChecked(false);
    //for debug.
    ui->run_algrunmodeBox->setCurrentIndex(tempstate.algTriggle);
    ui->run_algsourceBox->setCurrentIndex(tempstate.algImgsrc);
}

void Widget::LoadConnectionConfig()
{
    NetworkStr tempconfig = network->getNetworkConfig();
    ui->ports_ipaddressLineEdit->setText(tempconfig.value.ports_ipaddress);
    ui->ports_netmaskLineEdit->setText(tempconfig.value.ports_netmask);
    ui->ports_gatewayLineEdit->setText(tempconfig.value.ports_gateway);
    ui->ports_primarynameserverLineEdit->setText(tempconfig.value.ports_dns);
    ui->ports_macaddressLineEdit->setText(tempconfig.value.ports_mac);
    ui->ports_ftpserverLineEdit->setText(tempconfig.value.ports_ftpserverip);
    ui->ports_ftpserverportLineEdit->setText(tempconfig.value.ports_ftpserverport);
    ui->ports_usernameLineEdit->setText(tempconfig.value.ports_ftpusername);
    ui->ports_passwordLineEdit->setText(tempconfig.value.ports_ftppassword);
    ui->ports_fileuploadpathLineEdit->setText(tempconfig.value.ports_ftpfoldername);
    //    ui->ports_rtspport1LineEdit->setText(QString::number(tempconfig.value.ports_rtspport,10));
    ui->ports_multicastcomboBox->setCurrentIndex(tempconfig.value.ports_rtspmulticast);
}

void Widget::LoadInfomationConfig()
{
    InformationStr tempconfig = config->getInformation();
    NetworkStr tempnetwork = network->getNetworkConfig();

    ui->resource_totalmemoryvalueLabel->setText(QString::number(tempconfig.value.resource_totalmem)+"\tMB");
    ui->resource_freememoryvalueLabel->setText(QString::number(tempconfig.value.resource_freemem)+"\tMB");
    ui->resource_usedmemoryvalueLabel->setText(QString::number(tempconfig.value.resource_totalmem-tempconfig.value.resource_freemem)+"\tMB");
    ui->resource_sharedmemoryvalueLabel->setText(QString::number(tempconfig.value.resource_sharedmem)+"\tMB");
    ui->resource_armvalueLabel->setText(QString::number(tempconfig.value.resource_corearmmem)+"\tMB");
    ui->resource_dspvalueLabel->setText(QString::number(tempconfig.value.resource_coredspmem)+"\tMB");
    ui->resource_m3videovalueLabel->setText(QString::number(tempconfig.value.resource_corem3videomem)+"\tMB");
    ui->resource_m3vpssvalueLabel->setText(QString::number(tempconfig.value.resource_corem3vpssmem)+"\tMB");
    ui->resource_totalstoragevalueLabel->setText(QString::number(tempconfig.value.resource_storagemem)+"\tMB");
    ui->resource_usedstoragevalueLabel->setText(QString::number(tempconfig.value.resource_storagemem-tempconfig.value.resource_storagefreemem)+"\tMB");

    ui->device_versionvalueLabel->setText(QString::number(tempconfig.value.device_version));
    ui->device_algvalueLabel->setText(QString::number(config->getState().algType,10));
    ui->device_macvalueLabel->setText(tempnetwork.value.ports_mac);
    ui->device_ipvalueLabel->setText(tempnetwork.value.ports_ipaddress);
    ui->device_armfrqvalueLabel->setText(QString::number(tempconfig.value.resource_corearmfreq)+"\tMhz");
    ui->device_dspfrqvalueLabel->setText(QString::number(tempconfig.value.resource_coredspfreq)+"\tMhz");
    ui->device_m3videofrqvalueLabel->setText(QString::number(tempconfig.value.resource_corem3videofreq)+"\tMhz");
    ui->device_m3vpssfrqvalueLabel->setText(QString::number(tempconfig.value.resource_corem3vpssfreq)+"\tMhz");

    memoryseries->append("M3 VPSS", tempconfig.value.resource_corem3vpssmem);
    memoryseries->append("M3 VIDEO", tempconfig.value.resource_corem3videomem);
    memoryseries->append("ARM Cortex-A8", tempconfig.value.resource_corearmmem);
    memoryseries->append("DSP 674x", tempconfig.value.resource_coredspmem);
    memoryseries->setLabelsVisible(true);
    foreach(QPieSlice *slice,memoryseries->slices())
    {
        slice->setLabelColor(Qt::white);
        slice->setLabelFont(QFont("Arial",10,QFont::Bold));
        //        slice->setBorderWidth(5);
        slice->setBorderColor(QColor(0,0,0,0));
    }

    storageseries->append("Used", tempconfig.value.resource_storagemem-tempconfig.value.resource_storagefreemem);
    storageseries->append("Unused", tempconfig.value.resource_storagefreemem);
    storageseries->setLabelsVisible(true);
    foreach(QPieSlice *slice,storageseries->slices())
    {
        slice->setLabelColor(Qt::white);
        slice->setLabelFont(QFont("Arial",10,QFont::Bold));
        //        slice->setBorderWidth(5);
        slice->setBorderColor(QColor(0,0,0,0));
    }
}

void Widget::LoadAccountConfig()
{
    ui->account_usernamevalueLabel->setText(network->getLogUserName());
    ui->account_authorityvalueLabel->setText(QString::number(network->getLogAuthority(),10));
}

void Widget::ResetRun()
{
    //ui->run_videoinputwidget->clearImage(0);
}

void Widget::ResetOptions()
{
    ui->security_userNameLineEdit->clear();
    ui->security_passwordLineEdit->clear();
    ui->security_confirmPasswordLineEdit->clear();
    ui->security_authorityadminButton->setChecked(false);
    ui->security_authorityviewerButton->setChecked(false);

    ResetFtpBrowser();
    ui->diagnostic_itemnamecurrentLabel->clear();
    ui->diagnostic_previewWidget->clearImage(0);
}

void Widget::ResetInformation()
{
    cpuloadTimer->stop();
    memoryseries->clear();
    storageseries->clear();
    cpuChart->clear();
}

void Widget::ResetAccount()
{
    ui->LoginSpecifiedIPlineEdit->clear();
    ui->Account_cameralist->clear();
}

void Widget::ResetFtpBrowser()
{
    ui->diagnostic_ftpbrowsertablewidget->clearContents();
    ui->diagnostic_ftpbrowsertablewidget->setRowCount(0);
}

void Widget::SetupVideo()
{
    //set video img buffer queue
    m_hBufQueue = new video_bufQueue(DEFAULT_IMG_HEIGHT*DEFAULT_IMG_WIDTH,5);

    videothread =new QThread;
    h264video = new H264Video(m_hBufQueue);
    //m_hVideo = new m_hVideo(1,&m_hBufQueue);
    h264video->moveToThread(videothread);
    //m_hVideo->moveToThread(videothread);
    connect(this,SIGNAL(videocontrol(int)),h264video,SLOT(H264VideoOpen(int)));
    connect(h264video,&H264Video::getImage,this,[=](int index){
        QImage img(DEFAULT_IMG_WIDTH,DEFAULT_IMG_HEIGHT,QImage::Format_Grayscale8);
        auto ret = m_hBufQueue->bufOutput(img.bits());
        if(ret)
        {
            if(index==VIDEO_SHOW_RUN)
            {
                if(SourceFlag==false)
                    ui->run_videoinputwidget->setImage(img.scaledToHeight(ui->run_videoinputwidget->height()),1);
                else
                    emit getImage_Source(img,1);
            }
            else if(index==VIDEO_SHOW_CAMERA)
            {
                QImage tmp;
                if(camera_videoinputwidget->ratio()>=1.778)
                    tmp = img.scaledToHeight(camera_videoinputwidget->height());
                else
                    tmp = img.scaledToWidth(camera_videoinputwidget->width());
                camera_videoinputwidget->setImage(tmp,0);
                //ui->camera_2A_AEWeight_preWidget->setActiveRegion(tmp.size());
            }
        }
        else
            qDebug()<<"no img buf in queue";
    });
    connect(h264video,SIGNAL(getImage_Camera(QImage)),this,SLOT(setVideoImage_Camera(QImage)));
    connect(h264video,SIGNAL(getImage_Run(QImage)),this,SLOT(setVideoImage_Run(QImage)));
    connect(h264video,SIGNAL(clearImage()),this,SLOT(clearVideoImage()));
    //    connect(h264video,SIGNAL(getVideoInfo(int,int,int)),this,SLOT(setVideoInfo(int,int,int)));
    connect(h264video,SIGNAL(sendToLog(QString)),this,SLOT(LogDebug(QString)));
    connect(h264video,&H264Video::closeVideo,this,[=](){network->sendConfigToServer(NET_MSG_GET_DISABLE_ENCODE,0);});
    videothread->start();
}

void Widget::SetupRun()
{
    SourceVideoWidget = new SourceVideo();
    connect(this,SIGNAL(getImage_Source(QImage,int)),SourceVideoWidget,SLOT(setImage(QImage,int)));
    connect(SourceVideoWidget,SIGNAL(SourceVideoClose()),this,SLOT(SourceVideoWidgetClose()));
    connect(ui->run_processmodeAlgBox,SIGNAL(stateChanged(int)),config,SLOT(setLoadAlg(int)));
    connect(ui->run_saveResultBox,SIGNAL(stateChanged(int)),this,SLOT(resultLog(int)));
    //connect(ui->run_algviewchangeBox,&QPushButton::clicked,[=](){network->sendConfigToServer(NET_MSG_OUTPUT_IMGSOURCE,1);});

    RunVideoImage=QImage(1280,720,QImage::Format_RGB888);

    ui->run_videoinputwidget->resizeImageCount(2);
    QDir savedir;
    if(savedir.mkdir("./save/"))
    {
        LogDebug("did not find save dir, creat save dir.");
        qDebug()<<"did not find save dir, creat save dir.";
    }
    else
    {
        LogDebug("find save dir.");
        qDebug()<<"find save dir.";
    }

    QDir logdir;
    if(logdir.mkdir("./log/"))
    {
        LogDebug("did not find log dir, create log dir.");
        qDebug()<<"did not find log dir, create log dir.";
    }
    else
    {
        LogDebug("find log dir.");
        qDebug()<<"find log dir.";
    }

    QDir loglogdir;
    if(loglogdir.mkdir("./log/log/"))
    {
        LogDebug("did not find log log dir, create log log dir.");
        qDebug()<<"did not find log log dir, create log log dir.";
    }
    else
    {
        LogDebug("find log log dir.");
        qDebug()<<"find log log dir.";
    }

    QDir logerrdir;
    if(logerrdir.mkdir("./log/err/"))
    {
        LogDebug("did not find log err dir, create log err dir.");
        qDebug()<<"did not find log err dir, create log err dir.";
    }
    else
    {
        LogDebug("find log err dir.");
        qDebug()<<"find log err dir.";
    }
}

void Widget::SetupLog()
{
    logbar=new QStatusBar(this);
    ui->sublistLayout->addWidget(logbar);
    logbar->setStyleSheet("QStatusBar{color:black;background-color:rgba(255,242,204,200);}");
    connect(logbar,SIGNAL(messageChanged(QString)),this,SLOT(logtimeout(QString)));
    effect=new QGraphicsOpacityEffect();
    effect->setOpacity(0.7);
    logbar->setGraphicsEffect(effect);
}

void Widget::SetupInformations()
{
    memoryseries = new QPieSeries();
    memoryseries->setHoleSize(0.3);

    QChart *chart = new QChart();
    chart->addSeries(memoryseries);
    chart->setBackgroundVisible(false);
    chart->legend()->hide();
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background-color:rgba(0,0,0,0);");
    ui->resource_memorychartlayout->addWidget(chartView);

    storageseries = new QPieSeries();
    storageseries->setHoleSize(0.3);

    QChart *storagechart = new QChart();
    storagechart->addSeries(storageseries);
    storagechart->setBackgroundVisible(false);
    storagechart->legend()->hide();
    QChartView *storageView = new QChartView(storagechart);
    storageView->setRenderHint(QPainter::Antialiasing);
    storageView->setStyleSheet("background-color:rgba(0,0,0,0);");
    ui->resource_storagechartlayout->addWidget(storageView);

    cpuloadTimer = new QTimer;
    cpuloadTimer->setInterval(1000);
    cpuloadTimer->setSingleShot(false);
    cpuChart = new RTChart("CPU PayLoad",2);
    cpuChart->setDataRange(0,100);
    connect(ui->sublist,SIGNAL(currentChanged(int)),this,SLOT(sublistService(int)));
    connect(cpuloadTimer,SIGNAL(timeout()),this,SLOT(cpuloadUpdate()));
    ui->resource_cpuloadlayout->addWidget(cpuChart);
}

void Widget::SetupConnection()
{
    connect(this,SIGNAL(NetworkConfigtoServer()),network,SLOT(sendNetworkConfig()));
}

void Widget::SetupNetwork()
{
    network = new NetWork();
    connect(network,SIGNAL(udpRcvDataInfo(uchar*,int,int,int)),SLOT(frameFromSensor(uchar*,int,int,int)));
    connect(network,SIGNAL(udpRcvDataInfo(QByteArray)),SerialPortWidget,SLOT(udpDataRcv(QByteArray)));
    //    connect(ui->run_videosavesensorButton,SIGNAL(clicked()),network,SLOT(GetFrameFromSensor()));
    connect(ui->run_videosavesensorButton,&QPushButton::clicked,this,[=](){
        QString time =QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
        qDebug()<<time<<time.size();
        network->sendConfigToServer(NET_MSG_GET_FRAME_IMG,time.toLatin1().data(),time.size());
    });
    connect(SerialPortWidget,SIGNAL(setDebugMode(int)),this,SLOT(setDebugMode(int)));
    connect(ui->LogoutButton,SIGNAL(clicked()),network,SLOT(disconnectConnection()));
    connect(network,SIGNAL(AccountUpdate()),this,SLOT(UpdateUserAccountList()));
    connect(network,SIGNAL(sendToLog(QString)),this,SLOT(LogDebug(QString)));
    connect(network,SIGNAL(loginFailed()),this,SLOT(recoverLoginState()));
    connect(network,SIGNAL(loginSuccess(uint8_t)),this,SLOT(LoginSuccess(uint8_t)));
    connect(network,SIGNAL(disconnectfromServer()),this,SLOT(ConnectionLost()));
    //    connect(config,SIGNAL(sendToServer(_NET_MSG,QString)),network,SLOT(sendConfigToServer(_NET_MSG,QString)));
    connect(config,SIGNAL(sendToServer(_NET_MSG,unsigned char)),network,SLOT(sendConfigToServer(_NET_MSG,unsigned char)));
    connect(config,SIGNAL(sendToServer(_NET_MSG,unsigned char,unsigned char)),network,SLOT(sendConfigToServer(_NET_MSG,unsigned char,unsigned char)));
    connect(config,SIGNAL(sendToServerEntire(QVariant)),network,SLOT(sendConfigToServerEntire(QVariant)));
    connect(config,SIGNAL(sendToServerEntireForBoot(QVariant)),network,SLOT(sendConfigToServerEntireForBoot(QVariant)));
    connect(config,SIGNAL(sendToServerALG(QVariant)),network,SLOT(sendConfigToServerALG(QVariant)));
    connect(config,SIGNAL(sendToServerH3A(QVariant)),network,SLOT(sendConfigToServerH3A(QVariant)));
    connect(network,SIGNAL(sendAlgRsult(QByteArray)),this,SLOT(algresultUpdate(QByteArray)));

}

void Widget::SetupAccount()
{
    connect(network,SIGNAL(cameraDevice(int,QString)),this,SLOT(UpdateCameraList(int,QString)));
    ui->LoginSpecifiedIPlineEdit->setVisible(false);
    ui->LoginSpecifiedIPLabel->setVisible(false);
    //emit network->cameraScan();
    ui->Accountlist->setCurrentIndex(0);
}

void Widget::VideoCMD(H264Video* video,int cmd)
{
    if(!video->isVideoInit())
        network->sendConfigToServer(NET_MSG_GET_ENABLE_ENCODE,0);
    video->changeVideoStatus();
    emit videocontrol(cmd);
}

void Widget::LogDebug(QString msg)
{
    logbar->showMessage(msg,2000);
}

void Widget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    //    QPainterPath path;
    //    path.setFillRule(Qt::WindingFill);
    //    path.addRect(0, 0, this->width()+20, this->height()+20);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    //    painter.fillPath(path, QBrush(Qt::white));

    QColor color(0, 0, 0, 50);
    for(int i=0; i<5; i++)
    {
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        path.addRect(i, i, this->width()+(-i)*2, this->height()+(-i)*2);
        color.setAlpha(i*40);
        painter.setPen(color);
        painter.drawPath(path);
    }
}

void Widget::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    SerialPortWidget->close();
    SourceVideoWidget->close();

}

int Widget::countFlag(QPoint p,int row)
{
    if(p.y()<MARGIN_L)
        return 10+row;
    else if(p.y()>this->height()-MARGIN_S)
        return 30+row;
    else
        return 20+row;
}
int Widget::countRow(QPoint p)
{
    return p.x()>(this->width()-MARGIN_S)?2:1;
}

void Widget::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton)
    {
        this->isLeftPressed=true;
        QPoint temp=event->globalPos();
        pLast=temp;
        curPos=countFlag(event->pos(),countRow(event->pos()));
        event->ignore();
    }
}
void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    if(isLeftPressed)
        isLeftPressed=false;
    QApplication::restoreOverrideCursor();
    event->ignore();
}
void Widget::mouseMoveEvent(QMouseEvent *event)
{
    int poss=countFlag(event->pos(),countRow(event->pos()));
    if(poss==22)
        setCursor(Qt::SizeHorCursor);
    else if(poss==31)
        setCursor(Qt::SizeVerCursor);
    //    else if(poss==32)
    //        setCursor(Qt::SizeFDiagCursor);
    else
        setCursor(Qt::ArrowCursor);
    if(isLeftPressed)
    {
        QPoint ptemp=event->globalPos();
        ptemp=ptemp-pLast;
        if(curPos==11)
        {
            ptemp=ptemp+pos();
            move(ptemp);
        }
        else
        {
            QRect wid=geometry();
            switch (curPos)
            {
            case 22:wid.setRight(wid.right()+ptemp.x());
                break;
            case 31:wid.setBottom(wid.bottom()+ptemp.y());
                break;
                //            case 32:wid.setBottomRight(wid.bottomRight()+ptemp);
                //                break;
            }
            setGeometry(wid);
        }
        pLast=event->globalPos();
    }
    event->ignore();
}
void Widget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton&&countFlag(event->pos(),countRow(event->pos()))==11)
    {
        if(windowState()!=Qt::WindowFullScreen)
        {
            ui->mainwidgetLayout->setContentsMargins(0,0,0,0);
            setWindowState(Qt::WindowFullScreen);
        }
        else
        {
            ui->mainwidgetLayout->setContentsMargins(4,4,4,4);
            this->setGeometry(QApplication::desktop()->screenGeometry().width()/6,QApplication::desktop()->screenGeometry().height()/6,1280,720);
        }

    }
    event->ignore();
}

void Widget::SetupList()
{

    item_Run = addListItem(QString("  Run"),ui->mainlist,QIcon(":/image/source/Run.png"));
    item_Calibrate = addListItem(QString("  Calibrate"),ui->mainlist,QIcon(":/image/source/Calibrate.png"));
    item_Connection = addListItem(QString("  Connection"),ui->mainlist,QIcon(":/image/source/Connection.png"));
    item_Options = addListItem(QString("  Options"),ui->mainlist,QIcon(":/image/source/options.png"));
    item_Information = addListItem(QString("  Information"),ui->mainlist,QIcon(":/image/source/Information.png"));

    ui->mainlist->setFocusPolicy(Qt::NoFocus);
    setStyleSheet("#mainlist {background-color: rgba(0,0,0,0);border:0px}");
    ui->mainlist->setStyleSheet("QListWidget::Item {color:rgba(150,150,150,255);border-style:outset;"
                                "border-left-width:3px;border-color:rgba(0,0,0,0);}"
                                "QListWidget::Item:hover {color:rgba(255,255,255,255);"
                                "background-color: rgba(0,0,0,0);}"
                                "QListWidget::Item:selected {color:rgba(255,255,255,255);border-style:outset;"
                                "border-left-width: 3px;border-color: rgba(192,0,0,255);"
                                "background-color: rgba(50,50,50,200);}");
    connect(ui->mainlist,SIGNAL(currentRowChanged(int)),ui->sublist,SLOT(setCurrentIndex(int)));
}
QListWidgetItem* Widget::addListItem(QString name, QListWidget* list, QIcon icon=QIcon())
{
    QListWidgetItem* newitem = new QListWidgetItem(name,list);
    newitem->setSizeHint(QSize(newitem->sizeHint().rwidth(),50));
    newitem->setFont(QFont("Arial",15,QFont::Bold));
    newitem->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    newitem->setIcon(icon);
    return newitem;
}
void Widget::freeList()
{
    freeListItem(item_Calibrate);
    freeListItem(item_Connection);
    freeListItem(item_Run);
    freeListItem(item_Options);
    freeListItem(item_Information);
}

void Widget::freeListItem(QListWidgetItem* item)
{
    delete item;
}

void Widget::on_button_Account_clicked()
{
    ui->sublist->setCurrentIndex(5);
}

void Widget::on_button_maxsize_clicked()
{
    if(windowState()!=Qt::WindowFullScreen)
    {
        ui->mainwidgetLayout->setContentsMargins(0,0,0,0);
        setWindowState(Qt::WindowFullScreen);
    }
    else
    {
        ui->mainwidgetLayout->setContentsMargins(4,4,4,4);
        setWindowState(Qt::WindowNoState);
    }

}

void Widget::on_button_minsize_clicked()
{
    this->showMinimized();
}

void Widget::on_LoginButton_clicked()
{
    //
    //�??�账?��???��??
    //
    if(ui->LoginSpecifiedIPlineEdit->text()!="")
    {
        NetworkStr config = network->getNetworkConfig();
        config.value.ports_ipaddress = ui->LoginSpecifiedIPlineEdit->text();
        network->setNetworkConfig(config);
        LogDebug(QString("use specified IP address:%1").arg(ui->LoginSpecifiedIPlineEdit->text()));
        qDebug()<<QString("use specified IP address:%1").arg(ui->LoginSpecifiedIPlineEdit->text());
    }
    network->Login(ui->usernameLineEdit->text(),ui->passwordLineEdit->text());
    ui->LoginButton->setEnabled(false);
}

void Widget::on_button_Save_clicked()
{
    config->UpdateConfig(network->getUserName());
    UpdateConfigFile();

}

void Widget::on_button_SaveAs_clicked()
{
    SaveasMenu->exec(ui->button_SaveAs->mapToGlobal(QPoint(-15,ui->button_SaveAs->height())));
}

void Widget::on_system_configlist_clicked(const QModelIndex &index)
{
    ui->system_configcurrentlabel->setText(ui->system_configlist->item(index.row(),0)->text());
}

void Widget::on_system_configloadButton_clicked()
{
    if(ui->system_configcurrentlabel->text()!="")
    {
        config->OpenConfig(ui->system_configcurrentlabel->text());
        config->SetConfig();
        LoadFullConfig();
        UpdateConfigFile();
    }
    else
    {
        LogDebug("load failed, select config file first");
        qDebug()<<"load failed, select config file first";
    }
}

void Widget::ConfigSaveAs()
{
    SaveasMenu->hide();
    config->SaveAsConfig(SaveasComboBox->currentText(),network->getUserName());
    UpdateConfigFile();
    UpdateConfigFile();
}

void Widget::ConfigDelete()
{
    LogDebug("ready to delete");
    qDebug()<<"ready to delete";
    QPushButton* pbutton = qobject_cast<QPushButton*>(sender());
    int row=ui->system_configlist->indexAt(QPoint(pbutton->frameGeometry().x(),pbutton->frameGeometry().y())).row();
    if(ui->system_configlist->item(row,0)->text()==config->getCurrentConfigName())
    {
        LogDebug("could not delete loaded config");
        qDebug()<<"could not delete loaded config";
    }
    else
    {
        QFile f;
        f.remove(config->getConfigDir()+ui->system_configlist->item(row,0)->text()+".ini");
        UpdateConfigFile();
    }
}

void Widget::UserAdd()
{
    if(ui->security_passwordLineEdit->text()==ui->security_confirmPasswordLineEdit->text())
    {
        if(ui->security_authorityadminButton->isChecked()&&(!ui->security_authorityviewerButton->isChecked()))
            network->AddUser(ui->security_userNameLineEdit->text(),ui->security_passwordLineEdit->text(),0);
        else if((!ui->security_authorityadminButton->isChecked())&&ui->security_authorityviewerButton->isChecked())
            network->AddUser(ui->security_userNameLineEdit->text(),ui->security_passwordLineEdit->text(),1);
        else
        {
            LogDebug("choose authority first.");
            qDebug()<<"choose authority first.";
            return;
        }
    }
    else
    {
        LogDebug("Can not confirm password, adduser failed.");
        qDebug()<<"Can not confirm password, adduser failed.";
        return;
    }
}

void Widget::setVideoImage_Run(QImage image)
{
    if (image.height()>0)
    {
        RunVideoImage=image;
        if(SourceFlag==false)
            ui->run_videoinputwidget->setImage(image.scaledToHeight(ui->run_videoinputwidget->height()),1);
        else
            emit getImage_Source(image,1);
    }
}

void Widget::logtimeout(QString msg)
{
    if(msg=="")
    {
        effect->setOpacity(0.1);
        logbar->setGraphicsEffect(effect);
    }
    else
    {
        effect->setOpacity(0.7);
        logbar->setGraphicsEffect(effect);
    }
}

void Widget::LoginSuccess(uint8_t authority)
{
    //    h264video->setH264VideoUrl("rtsp://192.168.1.88:8554/h264ESVideoTest");
    config->OpenConfig(QString("fromCamera"));
    ConfigStr configFromServer;
    if(!network->GetSysinfo(&configFromServer))
    {
        emit LogDebug("GetSysinfo failed.");
        qDebug()<<"GetSysinfo failed.";
        recoverLoginState();
        return;
    }
    //set rtsp for h264
    LogDebug(QString("get rtsp url: %1").arg(network->getRTSPurl()));
    qDebug()<<"get rtsp url: "<<network->getRTSPurl();
    h264video->setH264VideoUrl(network->getRTSPurl());
    //set config with specified config from server and load ui params
    config->SetConfig(configFromServer);//reload
    config->initAlgService();

    if(config->getAlgConfigSize()!=0)
    {
        void* algparam = (void*)malloc(config->getAlgConfigSize());
        //qDebug()<<config->getAlgConfigSize();
        if(network->GetParams(NET_MSG_IMGALG_GET_PARAM,algparam,config->getAlgConfigSize()))
            config->setAlgConfig(algparam);
        free(algparam);
    }
    else
    {
        LogDebug("invalid ALG config size");
        qDebug()<<"invalid ALG config size";
    }
    LoadFullConfig();

    //load menu
    if(AUTHORITY_ADMIN==authority)
    {
        ui->mainlist->item(0)->setHidden(false);
        ui->mainlist->item(1)->setHidden(false);
        ui->mainlist->item(2)->setHidden(false);
        ui->mainlist->item(3)->setHidden(false);
        ui->mainlist->item(4)->setHidden(false);
        //update account list for administer
        UpdateUserAccountList();
    }
    else
    {
        ui->mainlist->item(0)->setHidden(false);
        ui->mainlist->item(1)->setHidden(true);
        ui->mainlist->item(2)->setHidden(true);
        ui->mainlist->item(3)->setHidden(true);
        ui->mainlist->item(4)->setHidden(false);
    }
    ui->mainlist->setVisible(true);
    ui->Accountlist->setCurrentIndex(1);
    ui->button_Save->setEnabled(true);
    ui->button_SaveAs->setEnabled(true);
    ui->LoginButton->setEnabled(true);
    //update config list file browser
    UpdateConfigFile();

}

void Widget::ConnectionLost()
{
    if(ui->Accountlist->currentIndex()==1)
    {
        //
        //?��?��?????�??��?��?�?�?
        //
        VideoCMD(h264video,VIDEO_TERMINATE);
        //close ftp
        emit network->ftp_close();
        //close result socket;
        network->closeAlgResultService();
        //ui operation
        ui->sublist->setCurrentIndex(5);
        ui->mainlist->setVisible(false);
        ui->Accountlist->setCurrentIndex(0);
        ui->button_Save->setEnabled(false);
        ui->button_SaveAs->setEnabled(false);
        if(ui->mainlist->currentRow()!=-1)
        {
            ui->mainlist->selectedItems()[0]->setSelected(false);
            ui->mainlist->setCurrentRow(-1);
        }
        ResetRun();
        ResetOptions();
        ResetInformation();
        ResetAccount();
    }
}


void Widget::recoverLoginState()
{
    ui->LoginButton->setEnabled(true);
}

void Widget::setVideoImage_Camera(QImage image)
{
    if (image.height()>0)
    {
        QImage temp;
        if(camera_videoinputwidget->ratio()>=1.778)
            temp = image.scaledToHeight(camera_videoinputwidget->height());
        else
            temp =  image.scaledToWidth(camera_videoinputwidget->width());
        camera_videoinputwidget->setImage(temp,0);
        //ui->camera_2A_AEWeight_preWidget->setActiveRegion(temp.size());
    }
}

void Widget::on_camera_videocontrolButton_clicked()
{
    VideoCMD(h264video,VIDEO_SHOW_CAMERA);
    QImage img(DEFAULT_IMG_WIDTH,DEFAULT_IMG_HEIGHT,QImage::Format_RGB888);
    img.fill(QColor(0,0,0));
    QPainter painter(&img);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    QPen pen = painter.pen();
    pen.setColor(Qt::white);
    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(18);
    painter.setPen(pen);
    painter.setFont(font);
    painter.drawText(img.width()/2,img.height()/2,"Loading");
    ui->run_videoinputwidget->setImage(img.scaledToHeight(ui->run_videoinputwidget->height()),1);
}

void Widget::clearVideoImage()
{
    LogDebug("clear image labels.");
    qDebug()<<"clear image labels.";
    //    ui->camera_videoinputlabel->clear();
    //    ui->run_videoinputlabel->clear();
    ui->run_videoinputwidget->clearImage(1);
    ui->run_videoinputwidget->clearImage(0);
    //ui->camera_videoinputwidget->clearImage();
    camera_videoinputwidget->clearImage(0);
    //    ui->summary_statecodelabel->setText("State Code: ");
    //    ui->summary_warningcodelabel->setText("Warning Code: ");
}

void Widget::on_run_videocontrolButton_clicked()
{
    VideoCMD(h264video,VIDEO_SHOW_RUN);

}

void Widget::on_run_videosourceButton_clicked()
{
    SourceFlag=true;
    SourceVideoWidget->clearImage();
    SourceVideoWidget->show();
}

void Widget::SourceVideoWidgetClose()
{
    SourceFlag=false;
}

void Widget::on_run_processmodeLoadButton_clicked()
{
    config->OpenConfig(ui->run_processmodeConfigBox->currentText());
    config->SetConfig();
    LoadFullConfig();
    UpdateConfigFile();
}

void Widget::deleteUser()
{
    QPushButton* delbutton = qobject_cast<QPushButton*>(sender());
    int row=ui->security_tablewidget->indexAt(QPoint(delbutton->frameGeometry().x(),delbutton->frameGeometry().y())).row();
    network->DeleteUser(ui->security_tablewidget->item(row,0)->text());
}

void Widget::on_system_configbootdefaultButton_clicked()
{
    QString cfgname = config->getCurrentConfigName();
    if(ui->system_configcurrentlabel->text()=="")
    {
        LogDebug("set default boot config failed, select config file first");
        qDebug()<<"set default boot config failed, select config file first";
        return;
    }
    config->OpenConfig(ui->system_configcurrentlabel->text());
    config->setConigAsBoot();
    config->OpenConfig(cfgname);
}


void Widget::on_ports_setButton_clicked()
{
    NetworkStr tempconfig=network->getNetworkConfig();
    tempconfig.value.ports_ipaddress=ui->ports_ipaddressLineEdit->text();
    tempconfig.value.ports_netmask=ui->ports_netmaskLineEdit->text();
    tempconfig.value.ports_gateway=ui->ports_gatewayLineEdit->text();
    tempconfig.value.ports_dns=ui->ports_primarynameserverLineEdit->text();
    tempconfig.value.ports_mac=ui->ports_macaddressLineEdit->text();
    tempconfig.value.ports_ftpserverip=ui->ports_ftpserverLineEdit->text();
    tempconfig.value.ports_ftpserverport=ui->ports_ftpserverportLineEdit->text();
    tempconfig.value.ports_ftpusername=ui->ports_usernameLineEdit->text();
    tempconfig.value.ports_ftppassword=ui->ports_passwordLineEdit->text();
    tempconfig.value.ports_ftpfoldername=ui->ports_fileuploadpathLineEdit->text();
    //    tempconfig.value.ports_rtspport=ui->ports_rtspport1LineEdit->text().toInt();
    tempconfig.value.ports_rtspmulticast=ui->ports_multicastcomboBox->currentIndex();
    network->setNetworkConfig(tempconfig);
    emit NetworkConfigtoServer();
}

void Widget::on_security_addusercontrolcancelButton_clicked()
{
    ui->security_userNameLineEdit->clear();
    ui->security_passwordLineEdit->clear();
    ui->security_confirmPasswordLineEdit->clear();
    ui->security_authorityadminButton->setChecked(false);
    ui->security_authorityviewerButton->setChecked(false);
}

void Widget::cpuloadUpdate()
{
    EzcpuDynamicParam cpuload;
    if(network->GetCpuloadresult(&cpuload))
    {
        int minLoad,maxLoad;
        if(cpuload.prf_a8>cpuload.prf_dsp)
        {
            minLoad=cpuload.prf_dsp;
            maxLoad=cpuload.prf_a8;
        }
        else
        {
            minLoad=cpuload.prf_a8;
            maxLoad=cpuload.prf_dsp;
        }
        minLoad=(minLoad-5)>0?(minLoad-5):0;
        maxLoad=(maxLoad+5)<100?(maxLoad+5):100;
        cpuChart->setDataRange(minLoad,maxLoad);
        cpuChart->updateData(cpuload.prf_a8,0);
        cpuChart->updateData(cpuload.prf_dsp,1);
    }
    else
    {
        LogDebug("CPU load failed");
        qDebug()<<"CPU load failed";
        cpuloadTimer->stop();
    }
}

void Widget::algresultUpdate(QByteArray ba)
{
    // unsigned int position,distance;
    unsigned logSize = 0;
    int rectNum = 0;
    if(config->getAlgResultSize()!=0)
    {
        if(isRunOnScreen==true)
        {
            //qDebug()<<ba.count()<<config->getAlgResultSize();

            void* tempresult = (void*)malloc(config->getAlgResultSize());
            memcpy(tempresult,ba.data(),config->getAlgResultSize());

            rectNum = *(int *)((char*)tempresult+32);
            logSize = config->getAlgResultSize() - (8*(50 - rectNum));
            int flag = *(int*)(tempresult+logSize);
            QString strFlag="";
            flag==1?strFlag="BAUME":strFlag="SSZ";
            ui->tempLabel->setText(strFlag);

            if(resultFile!=NULL&&(*(int*)((char*)tempresult+8))!=0)
                //           if(resultFile!=NULL)
                fwrite(tempresult,logSize,1,resultFile);

            config->reflashAlgResult(tempresult);
            if(SourceFlag==false)
                ui->run_videoinputwidget->setImage(config->refreshAlgImage(),0,SCALE_HEIGHT);
            else
                emit getImage_Source(config->refreshAlgImage(),0);
            free(tempresult);
        }
    }
    else
    {
        LogDebug("invalid ALG result size");
        qDebug()<<"invalid ALG result size";
    }
}

void Widget::camera_2AmodecomboBox_Service(int index)
{
    if(index==0||index==2)
    {
        ui->camera_exposurelineEdit->setEnabled(true);
        ui->camera_exposureSlider->setEnabled(true);
        ui->camera_pipegainlineEdit->setEnabled(true);
        ui->camera_pipegainSlider->setEnabled(true);
        ui->camera_sensorgainlineEdit->setEnabled(true);
        ui->camera_sensorgainSlider->setEnabled(true);
    }
    else
    {
        ui->camera_exposurelineEdit->setEnabled(false);
        ui->camera_exposureSlider->setEnabled(false);
        ui->camera_pipegainlineEdit->setEnabled(false);
        ui->camera_pipegainSlider->setEnabled(false);
        ui->camera_sensorgainlineEdit->setEnabled(false);
        ui->camera_sensorgainSlider->setEnabled(false);
    }
    unsigned char tempexposure;
    unsigned short tempgain;
    network->getConfigFromSever(NET_MSG_GET_EXPOSURE,&tempexposure);
    network->getConfigFromSever(NET_MSG_GET_GAIN,&tempgain,2);
    //tempgain=tempgain&0xffff;
    ui->camera_exposurelineEdit->editValue(tempexposure);
    ui->camera_sensorgainlineEdit->editValue(tempgain>>8);
    ui->camera_pipegainlineEdit->editValue(tempgain&0xff);
    ConfigStr tempconfig;
    tempconfig = config->getConfig();
    tempconfig.calibrate.value.camera_Exposure=tempexposure;
    tempconfig.calibrate.value.camera_SensorGain=tempgain>>8;
    tempconfig.calibrate.value.camera_PipeGain=tempgain&0xff;
    qDebug()<<"sensor gain is: "<<tempconfig.calibrate.value.camera_SensorGain<<" pipe gain is: "<<tempconfig.calibrate.value.camera_PipeGain;
    config->SetConfig(tempconfig,CONFIG_SAVEONLY);
}

void Widget::sublistService(int index)
{
    if(index==4)
    {
        //        LogDebug("enable cpu load");
        //qDebug()<<"enable cpu load";
        cpuloadTimer->start();
    }
    else
    {
        //        LogDebug("disable cpu load");
        //qDebug()<<"disable cpu load";
        cpuloadTimer->stop();
    }
    if(index==0)
    {
        //qDebug()<<"enable alg result";
        network->openAlgResultService();
        isRunOnScreen=true;
    }
    else
    {
        //qDebug()<<"disable alg result";
        isRunOnScreen=false;
    }
}
void Widget::on_run_algsourceBox_activated(int index)
{
    if(index==0)
    {
        network->sendConfigToServer(NET_MSG_IMGALG_STATIC_IMG,1);
    }
    else
    {
        network->sendConfigToServer(NET_MSG_IMGAL_SENSOR_IMG,1);
    }
}

void Widget::frameFromSensor(uchar *data, int width,int height,int pitch)
{
    QString time =QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    //    FILE fp;
    //    QString filename = "D:/srcimg"+data+""
    //    fp=fopen("D:/srcimg"+time+".h","wb");
    //    uchar* pdata=data;
    //    for(int i=0;i<height;i++)
    //    {
    //        pdata =data+i*pitch;
    //        fwrite(pdata,1,width,fp);
    //    }
    //    fclose(fp);
    QImage frame(data,width,height,pitch,QImage::Format_Grayscale8);
    if(frame.save("./save/"+time+"_sensor.jpg"))
    {
        LogDebug("save image from sensor successfully.");
        qDebug()<<"save image from sensor successfully.";
    }
    else
    {
        LogDebug("failed to save image from sensor.");
        qDebug()<<"failed to save image from sensor.";
    }
}

void Widget::setDebugMode(int index)
{
    if(index==0)
    {
        network->sendConfigToServer(NET_MSG_DEBUG_NONE,1);
    }
    else if(index==1)
    {
        network->sendConfigToServer(NET_MSG_DEBUG_SERIAL,1);
    }
    else if(index==2)
    {
        network->setSocketDebugMode();
    }
}

void Widget::firmwareUpdateService(int cmd)
{
    QString localFilePath = "./update/";
    QString targetPath;
    switch(cmd)
    {
    case 0://FIRMWARE_C6DSP to xe674
    {
        targetPath=FIRMWARE_C6DSP;
        localFilePath += "ipnc_rdk_fw_c6xdsp.xe674";
        break;
    }
    case 1:
    {
        targetPath=FIRMWARE_VPSSM3;
        localFilePath += "ipnc_rdk_fw_m3vpss.xem3";
        break;
    }
    case 2:
    {
        targetPath=FIRMWARE_VIDEOM3;
        localFilePath += "ipnc_rdk_fw_m3video.xem3";
        break;
    }
    case 3:
    {
        targetPath=SDS_EZFTP_PATH;
        localFilePath += "Ezftp";
        break;
    }
    case 4:
    {
        targetPath=SDS_SERIAL_PATH;
        localFilePath += "EzApp";
        break;
    }
    case 5:
    {
        targetPath=SDS_MCFW_PATH;
        localFilePath += "ipnc_rdk_mcfw.out";
        break;
    }
    case 6:
    {
        targetPath=SDS_APP_SERVER;
        localFilePath += "system_server";
        break;
    }
    case 7:
    {
        targetPath=SDS_TEST_PATH;
        localFilePath += "test";
        break;
    }
    default:
    {
        qDebug()<<"unknown update cmd!";
        return;
    }
    }
    QFileInfo localFile(localFilePath);
    if(localFile.isFile())
    {
        FILE* fp=NULL;
        fp = fopen(localFilePath.toLatin1().data(),"rb");
        if(fp==NULL)
        {
            qDebug()<<"open failed";
            return;
        }
        int size = localFile.size();
        QByteArray ba;
        ba.resize(size);
        fread(ba.data(),1,size,fp);
        emit network->ftp_put(targetPath,ba);
        fclose(fp);
    }
    else
        qDebug()<<"could not find specified update file";
}

void Widget::loadALGConfigUi(QVBoxLayout *layout, QPoint pos)
{

    ui->algorithm_settinglayout->addWidget(layout->parentWidget(),pos.x(),pos.y());
}

void Widget::loadALGResultUi(QVBoxLayout *layout, QPoint pos)
{
    QVBoxLayout* temp = (QVBoxLayout*)ui->run_resulttabWidget->widget(pos.x())->layout();
    temp->insertWidget(pos.y(),layout->parentWidget());
}

void Widget::on_algorithm_uploadButton_clicked()
{
    if(config->getAlgConfigSize()!=0)
    {
        void* param = (void*)malloc(config->getAlgConfigSize());
        config->getAlgConfig(param);
        network->sendConfigToServer(NET_MSG_IMGALG_SET_PARAM,param,config->getAlgConfigSize());
        free(param);
    }
    else
    {
        LogDebug("invalid ALG config size");
        qDebug()<<"invalid ALG config size";
    }
}

void Widget::on_run_algrunmodeBox_activated(int index)
{
    if(index==0)
    {
        network->sendConfigToServer(NET_MSG_IMGALG_NORMAL_MODE,1);
    }
    else
    {
        network->sendConfigToServer(NET_MSG_IMGALG_DEBUG_MODE,1);
    }
}



void Widget::on_button_Reboot_clicked()
{
    network->sendConfigToServer(NET_MSG_RESET_SYSTEM,1);
}

void Widget::on_diagnostic_downloadButton_clicked()
{
    QString dir = ui->diagnostic_dirLineEdit->text();
    QString filename = ui->diagnostic_itemnamecurrentLabel->text();
    if(dir==""||filename=="")
    {
        qDebug()<<"invalid file path or invalid file name";
        return;
    }
    QImage img_save = *ftpPreView;
    if(!img_save.save(dir+"/"+filename+".jpg"))
        qDebug()<<"save failed";
    else
        qDebug()<<"save successfully";

}

void Widget::on_diagnostic_ftpbrowsertablewidget_clicked(const QModelIndex &index)
{
    if(ui->diagnostic_ftpbrowsertablewidget->item(index.row(),0)->text()!="...")
    {
        QString name = ui->diagnostic_ftpbrowsertablewidget->item(index.row(),0)->text();
        ui->diagnostic_itemnamecurrentLabel->setText(name);

        if(ui->diagnostic_ftpbrowsertablewidget->item(index.row(),2)->text()=="File")
        {
            ui->diagnostic_previewWidget->clearImage(0);
            ui->diagnostic_errorvalueLabel->clear();
            emit network->ftp_get(name);
        }
    }
}

void Widget::on_diagnostic_dirButton_clicked()
{
    QString dstName = QFileDialog::getExistingDirectory (this,tr("Open Directory"),"/home",QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks ) ;
    if(dstName!="")
        ui->diagnostic_dirLineEdit->setText(dstName);
}

void Widget::on_diagnostic_ftpbrowsertablewidget_doubleClicked(const QModelIndex &index)
{
    QString dir = network->ftp_curDir();
    //qDebug()<<dir;
    if("..."==ui->diagnostic_ftpbrowsertablewidget->item(index.row(),0)->text())
    {


        if(dir==DEFAULT_PATH)
        {
            qDebug()<<"root";
            return;
        }
        else
        {
            while(!(QString("/")==dir.at(dir.length()-1)))
            {
                dir.chop(1);
            }
            dir.chop(1);
            ResetFtpBrowser();
            emit network->ftp_list(dir);
        }
    }
    else if("Dir"==ui->diagnostic_ftpbrowsertablewidget->item(index.row(),2)->text())
    {
        dir.append("/");
        dir.append(ui->diagnostic_ftpbrowsertablewidget->item(index.row(),0)->text());
        ResetFtpBrowser();
        emit network->ftp_list(dir);
    }
}

void Widget::on_system_firmwareupdateButton_clicked()
{
    network->ftp_refresh();
    updateWidget->show();
    updateWidget->move((QApplication::desktop()->width() - updateWidget->width())/2,(QApplication::desktop()->height() - updateWidget->height())/2);
}

void Widget::on_Account_camerascanButton_clicked()
{
    ui->Account_cameralist->clear();
    emit network->cameraScan();
}

void Widget::on_camera_2A_ApplyButton_clicked()
{
    EzCamH3AParam temp;
    temp.targetBrightness = ui->camera_2A_TargetBrightnesslineEdit->text().toInt();
    temp.targetBrightnessMax=ui->camera_2A_MaxBrightnesslineEdit->text().toInt();
    temp.targetBrightnessMin=ui->camera_2A_MinBrightnesslineEdit->text().toInt();
    temp.threshold = ui->camera_2A_ThresholdlineEdit->text().toInt();
    config->saveH3AParams(temp);
    QVariant h3aparampack;
    h3aparampack.setValue(temp);
    emit config->sendToServerH3A(h3aparampack);



}

void Widget::on_Account_cameralist_itemClicked(QListWidgetItem *item)
{
    QString currentip=item->text();
    currentip=currentip.remove(0,currentip.indexOf("\t")+1);
    //    qDebug()<<currentip;
    //    NetworkStr config = network->getNetworkConfig();
    //    config.value.ports_ipaddress = currentip;
    //    network->setNetworkConfig(config);
    ui->LoginSpecifiedIPlineEdit->setText(currentip);
}

void Widget::on_camera_2A_AEWeight_getButton_clicked()
{
    EzCamH3AWeight weight;
    network->GetParams(NET_MSG_GET_2A_WEIGHT,&weight,sizeof(EzCamH3AWeight));
    //    ui->camera_2A_AEWeight_preWidget->weightReflash(weight);
    ui->camera_2A_AEWeight_width1lineEdit->setText(QString::number(weight.width1,10));
    ui->camera_2A_AEWeight_height1lineEdit->setText(QString::number(weight.height1,10));
    ui->camera_2A_AEWeight_h_start2lineEdit->setText(QString::number(weight.h_start2,10));
    ui->camera_2A_AEWeight_v_start2lineEdit->setText(QString::number(weight.v_start2,10));
    ui->camera_2A_AEWeight_width2lineEdit->setText(QString::number(weight.width2,10));
    ui->camera_2A_AEWeight_height2lineEdit->setText(QString::number(weight.height2,10));
    ui->camera_2A_AEWeight_weightlineEdit->setText(QString::number(weight.weight,10));

}

void Widget::on_camera_2A_AEWeight_uploadButton_clicked()
{
    EzCamH3AWeight weight;
    weight.width1=ui->camera_2A_AEWeight_width1lineEdit->text().toInt();
    weight.height1=ui->camera_2A_AEWeight_height1lineEdit->text().toInt();
    weight.h_start2=ui->camera_2A_AEWeight_h_start2lineEdit->text().toInt();
    weight.v_start2=ui->camera_2A_AEWeight_v_start2lineEdit->text().toInt();
    weight.width2=ui->camera_2A_AEWeight_width2lineEdit->text().toInt();
    weight.height2=ui->camera_2A_AEWeight_height2lineEdit->text().toInt();
    weight.weight=ui->camera_2A_AEWeight_weightlineEdit->text().toInt();
    ConfigStr tempconfig = config->getConfig();
    tempconfig.calibrate.value.camera_2A_weight_width1 = ui->camera_2A_AEWeight_width1lineEdit->text().toInt();
    tempconfig.calibrate.value.camera_2A_weight_height1 = ui->camera_2A_AEWeight_height1lineEdit->text().toInt();
    tempconfig.calibrate.value.camera_2A_weight_h_start2 = ui->camera_2A_AEWeight_h_start2lineEdit->text().toInt();
    tempconfig.calibrate.value.camera_2A_weight_v_start2 = ui->camera_2A_AEWeight_v_start2lineEdit->text().toInt();
    tempconfig.calibrate.value.camera_2A_weight_width2 = ui->camera_2A_AEWeight_width2lineEdit->text().toInt();
    tempconfig.calibrate.value.camera_2A_weight_height2 = ui->camera_2A_AEWeight_height2lineEdit->text().toInt();
    tempconfig.calibrate.value.camera_2A_weight_weight = ui->camera_2A_AEWeight_weightlineEdit->text().toInt();
    config->SetConfig(tempconfig,CONFIG_SAVEONLY);
    network->sendConfigToServer(NET_MSG_SET_2A_WEIGHT,&weight,sizeof(EzCamH3AWeight));
}

void Widget::on_camera_2A_AEWeight_previewButton_clicked()
{
    //    EzCamH3AWeight weight;
    //    weight.width1=ui->camera_2A_AEWeight_width1lineEdit->text().toInt();
    //    weight.height1=ui->camera_2A_AEWeight_height1lineEdit->text().toInt();
    //    weight.h_start2=ui->camera_2A_AEWeight_h_start2lineEdit->text().toInt();
    //    weight.v_start2=ui->camera_2A_AEWeight_v_start2lineEdit->text().toInt();
    //    weight.width2=ui->camera_2A_AEWeight_width2lineEdit->text().toInt();
    //    weight.height2=ui->camera_2A_AEWeight_height2lineEdit->text().toInt();
    //    weight.weight=ui->camera_2A_AEWeight_weightlineEdit->text().toInt();
    //    ui->camera_2A_AEWeight_preWidget->weightReflash(weight);
}

void Widget::on_LoginSpecifiedIPButton_clicked()
{
    if(ui->LoginSpecifiedIPlineEdit->isHidden())
    {
        ui->LoginSpecifiedIPlineEdit->setVisible(true);
        ui->LoginSpecifiedIPLabel->setVisible(true);
        ui->LoginSpecifiedIPButton->setStyleSheet("QPushButton#LoginSpecifiedIPButton{border-image: url(:/image/source/uparrow.png);}"
                                                  "QPushButton#LoginSpecifiedIPButton:hover{border-image: url(:/image/source/uparrow_active.png);}"
                                                  "QPushButton#LoginSpecifiedIPButton:pressed{border-image: url(:/image/source/uparrow_active.png);}");
    }
    else
    {
        ui->LoginSpecifiedIPlineEdit->setVisible(false);
        ui->LoginSpecifiedIPLabel->setVisible(false);
        ui->LoginSpecifiedIPButton->setStyleSheet("QPushButton#LoginSpecifiedIPButton{border-image: url(:/image/source/downarrow.png);}"
                                                  "QPushButton#LoginSpecifiedIPButton:hover{border-image: url(:/image/source/downarrow_active.png);}"
                                                  "QPushButton#LoginSpecifiedIPButton:pressed{border-image: url(:/image/source/downarrow_active.png);}");
    }
}

void Widget::on_camera_2A_AEWeight_paintButton_clicked()
{
    //    if(ui->camera_2A_AEWeight_preWidget->currentMode()==MODE_CUSTOM)
    //    {
    //        ui->camera_2A_AEWeight_preWidget->editRecover();
    //        ui->camera_2A_AEWeight_preWidget->changeMode(MODE_DEFAULT);
    //    }
    //    else
    //        ui->camera_2A_AEWeight_preWidget->changeMode(MODE_CUSTOM);
}

void Widget::getH3AWeight(EzCamH3AWeight cfg)
{
    ui->camera_2A_AEWeight_width1lineEdit->editValue(cfg.width1);
    ui->camera_2A_AEWeight_height1lineEdit->editValue(cfg.height1);
    ui->camera_2A_AEWeight_h_start2lineEdit->editValue(cfg.h_start2);
    ui->camera_2A_AEWeight_v_start2lineEdit->editValue(cfg.v_start2);
    ui->camera_2A_AEWeight_width2lineEdit->editValue(cfg.width2);
    ui->camera_2A_AEWeight_height2lineEdit->editValue(cfg.height2);
    ui->camera_2A_AEWeight_weightlineEdit->editValue(cfg.weight);
}

void Widget::on_algorithm_setdefaultButton_clicked()
{
    void *temp = malloc(config->getAlgConfigSize());
    config->getAlgConfig(temp);
    network->sendConfigToServer(NET_MSG_IMGALG_DEF_PARAM,temp,config->getAlgConfigSize());
    free(temp);
}

void Widget::resultLog(int state)
{
    if(state==2)
    {
        if(resultFile!=NULL)
        {
            fclose(resultFile);
            resultFile=NULL;
        }
        QString filename = "./log/err/";
        filename+=QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        filename+=".bin";
        resultFile = fopen(filename.toLatin1().data(),"w");

        //        filename.clear();
        //        filename =  "./log/log/";
        //        filename+=QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        //        filename+=".xls";
        //        resultLogFile = fopen(filename.toLatin1().data(),"w");
    }
    else if(state ==0)
    {
        if(resultFile!=NULL)
        {
            fflush(resultFile);
            fclose(resultFile);
            resultFile=NULL;
        }

        //        if(resultLogFile != NULL)
        //        {
        //            fclose(resultLogFile);
        //            resultLogFile=NULL;
        //        }
    }
}


void Widget::on_diagnostic_local_loadButton_clicked()
{
    network->ftp_refresh();

    QString dstName = QFileDialog::getOpenFileName(this,tr("Open file"),"/home",tr("Allfile(*.*)"),Q_NULLPTR,QFileDialog::DontResolveSymlinks) ;
    if(dstName!="")
    {
        m_hLocalAlgDebug->loadLocalImage(dstName);
    }
}

void Widget::on_diagnostic_deleteButton_clicked()
{
    emit network->ftp_del(ui->diagnostic_itemnamecurrentLabel->text());
}

void Widget::on_diagnostic_saveErrImgCheckBox_stateChanged(int stat)
{
    if(stat == 0)
        network->sendConfigToServer(NET_MSG_IMGALG_DISABLE_SAVE_ERRIMG,0);
    else if(stat == 2)
        network->sendConfigToServer(NET_MSG_IMGALG_SAVE_ERRIMG,0);
}
