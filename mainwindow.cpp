#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    miTimer = new QTimer(this);
    miTimerDatos = new QTimer(this);

    QSerialPort1=new QSerialPort(this);
    QSerialPort1->setPortName("COM6");
    QSerialPort1->setBaudRate(115200);
    QSerialPort1->setDataBits(QSerialPort::Data8);
    QSerialPort1->setParity(QSerialPort::NoParity);
    QSerialPort1->setFlowControl(QSerialPort::NoFlowControl); //parametros del puerto serie
    connect(QSerialPort1, &QSerialPort::readyRead, this, &MainWindow::onQSerialPort1Rx);//conecta para hacer la interrupcion de recepción de datos
    connect(miTimer,&QTimer::timeout,this,&MainWindow::miTimerOnTime);
    connect(miTimerDatos,&QTimer::timeout,this,&MainWindow::datosOnTime);
    ui->pushButton->setEnabled(false);
    ui->state->append("PUERTO CERRADO :: ESPERANDO CONEXION");


    miTimer->start(10);
    miTimerDatos->start(50);
    updateData=6;
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    switch(comandos){
        case OPENPORT:
                if(QSerialPort1->isOpen()){ //si el puerto esta abierto
                        QSerialPort1->close(); //lo cierra
                        ui->comboBox->setItemText(1,"Open Port");//si se pudo cerrar
                        ui->textBrowser->append("**************PUERTO CERRADO***************");
                        ui->state->setText("    ESPERANDO CONEXION...    ");
//                        miPaintBox->getCanvas()->fill(Qt::black);
//                        miPaintBox->update();
                    }
                    else{
                        if(QSerialPort1->open(QSerialPort::ReadWrite)){
                            ui->comboBox->setItemText(1,"close Port");//si se pudo abrir
                            ui->state->setText("CONEXION LISTA");
//                            miPaintBox->getCanvas()->fill(Qt::transparent);
//                            paint();//para empezar a dibujar
                        }else{
                            QMessageBox::information(this, "PORT", "NO se pudo abrir el PUERTO");
                        }
                    }

        break;
        case ALIVE:
            SetBufTX(ALIVE);
        break;
        case GET_IR:
            SetBufTX(GET_IR);
        break;
        case MOTOR_TEST:
            SetBufTX(MOTOR_TEST);
        break;
        case SERVO_TEST:
            SetBufTX(SERVO_TEST);
        break;
        case GET_DISTANCE:
            SetBufTX(GET_DISTANCE);
        break;
        case GET_SPEED:
            SetBufTX(GET_SPEED);
        break;
    }
}
void MainWindow::on_comboBox_currentIndexChanged(int index){
    switch (index) { //para los comandos de la combobox
        case CBWAITING:
            ui->pushButton->setEnabled(false);
        break;
        case CBOPENP:
            ui->pushButton->setEnabled(true);
            comandos=OPENPORT;
        break;
        case CBALIVE:
           if(QSerialPort1->isOpen()){
            ui->pushButton->setEnabled(true);
            comandos=ALIVE;
           }else{
             ui->pushButton->setEnabled(false);
           }
        break;
        case CBGETIR:
           if(QSerialPort1->isOpen()){
            ui->pushButton->setEnabled(true);
            comandos=GET_IR;
           }else{
             ui->pushButton->setEnabled(false);
           }
        break;
        case CBMOTORTEST:
           if(QSerialPort1->isOpen()){
            ui->pushButton->setEnabled(true);
            comandos=MOTOR_TEST;
           }else{
             ui->pushButton->setEnabled(false);
           }
        break;
        case CBSERVOTEST:
           if(QSerialPort1->isOpen()){
            ui->pushButton->setEnabled(true);
            comandos=SERVO_TEST;
           }else{
             ui->pushButton->setEnabled(false);
           }
        break;
        case CBGETDISTANCE:
           if(QSerialPort1->isOpen()){
            ui->pushButton->setEnabled(true);
            comandos=GET_DISTANCE;
           }else{
             ui->pushButton->setEnabled(false);
           }
        break;
        case CBGETSPEED:
           if(QSerialPort1->isOpen()){
            ui->pushButton->setEnabled(true);
            comandos=GET_SPEED;
           }else{
             ui->pushButton->setEnabled(false);
           }
        break;
    }
}
void MainWindow::miTimerOnTime(){
    if(bufRX.timeOut!=0){
        bufRX.timeOut--;
    }else{
        estado=START;
    }
}
void MainWindow::datosOnTime(){
    if(updateData!=0){
        updateData--;
    }else{
        updateData=6;
        if(QSerialPort1->isOpen())
            SetBufTX(GET_DISTANCE);
    }
}
void MainWindow::onQSerialPort1Rx(){
    unsigned char *reception;
    int count;
    QString str;
    count=QSerialPort1->bytesAvailable();

    if(count <= 0)
        return;

    reception= new unsigned char [count];

    QSerialPort1->read((char *)reception ,count);

    for(int i=0;i<count;i++){
     if(isalnum(reception[i]))
         str = str + QString("%1").arg((char )reception[i]);
     else
         str = str + "/" + QString("%1").arg(reception[i],2,16,QChar('0')) +"/";
    }
    ui->textBrowser->setTextColor(Qt::green);
    ui->textBrowser->append("MBED->PC: " +str ); //imprime el crudo
    bufRX.timeOut =5;
    for(int i=0;i<count;i++){
        switch (estado){
            case START:
                    if(reception[i]== 'U'){ //recibio la U
                        estado=HEADER_1;
                        bufRX.cheksum=0;
                    }
            break;
            case HEADER_1:
                if(reception[i]== 'N') //recibio la N
                    estado=HEADER_2;
                else{
                    i--;
                    estado=START;
                }
            break;
            case HEADER_2:
                if(reception[i]== 'E') //recibio la E
                    estado=HEADER_3;
                else{
                    i--;
                    estado=START;
                }
            break;
            case HEADER_3:
                if(reception[i]== 'R') //recibio la U
                    estado=NBYTES;
                else{
                    i--;
                    estado=START;
                }
            break;
            case NBYTES:
                bufRX.nBytes=reception[i];
                estado=TOKEN;
            break;
            case TOKEN:
                if(reception[i]==':'){
                    estado=PAYLOAD;
                    bufRX.cheksum= 'U' ^ 'N' ^ 'E' ^ 'R' ^ bufRX.nBytes ^ ':';
                    bufRX.payLoad[0]=bufRX.nBytes;
                    bufRX.index=1;
                }else{
                    i--;//token
                    estado=START;
                }
            break;
            case PAYLOAD:

                if(bufRX.nBytes>1){
                    bufRX.payLoad[bufRX.index++]=reception[i];
                    bufRX.cheksum^=reception[i];
                }
                bufRX.nBytes--;
                if(bufRX.nBytes==0){
                    estado=START;
                    if(bufRX.cheksum==reception[i]){
                        decodeData();
                    }
                }
            break;
        default:
            estado=START;
            break;
        }
    }
    delete [] reception;
}
void MainWindow::decodeData(){
    QString str ;

    ui->textBrowser->setTextColor(Qt::red);

        switch (bufRX.payLoad[1]){ //ID
            case ALIVE:
                  str="MBED -> PC ID VALIDO ACK";
                break;
            case GET_DISTANCE:
               str="MBED -> PC ID RECIBI DISTANCIA";
               myWord.ui8[0]=bufRX.payLoad[2];
               myWord.ui8[1]=bufRX.payLoad[3];
               myWord.ui8[2]=bufRX.payLoad[4];
               myWord.ui8[3]=bufRX.payLoad[5];
               distance=myWord.ui32;
               ui->lcdDistance->display(distance/58);
            break;
            case GET_IR:
                str="MBED -> PC ID RECIBI IR";
                myWord.ui8[0]=bufRX.payLoad[2];
                myWord.ui8[1]=bufRX.payLoad[3];
                myWord.ui8[2]=bufRX.payLoad[4];
                myWord.ui8[3]=bufRX.payLoad[5];
                irLeft=myWord.ui16[0];
                irRight=myWord.ui16[1];
                ui->lcdIr_0->display(irLeft);
                ui->lcdIr_0->display(irRight);
            break;
        default:
            str=((char *)bufRX.payLoad);
            str= ("MBED-->PC *ID Invalido * (" + str + ")");
            break;
            }

   ui->textBrowser->append(str);
}
void MainWindow::SetBufTX(uint8_t ID)
{
    bufTX.index=0;
    bufTX.payLoad[bufTX.index++]='U';
    bufTX.payLoad[bufTX.index++]='N';
    bufTX.payLoad[bufTX.index++]='E';
    bufTX.payLoad[bufTX.index++]='R';
    bufTX.payLoad[bufTX.index++]= 0 ;
    bufTX.payLoad[bufTX.index++]=':';//termina de armar el header
    switch (ID) {
        case ALIVE: //manda alive
            bufTX.payLoad[bufTX.index++]=ALIVE;
            bufTX.payLoad[NBYTES]=0x02;
        break;
        case GET_DISTANCE:
            bufTX.payLoad[bufTX.index++]=GET_DISTANCE;
            bufTX.payLoad[NBYTES]=0x02;
        break;
    }
    bufTX.cheksum=0;
    for(int i=0;i<bufTX.index;i++){
        bufTX.cheksum ^=bufTX.payLoad[i];
    }
    bufTX.payLoad[bufTX.index++]=bufTX.cheksum;

    if(QSerialPort1->isWritable()){
       QSerialPort1->write((char *)bufTX.payLoad,bufTX.payLoad[NBYTES]+6);
   }
   QString str;
   for(int i=0;i<bufTX.index;i++){
    //str = str + QString("%1").arg(bufTX.payLoad[i],2,16,QChar('0'));
    if(isalnum(bufTX.payLoad[i]))
        str = str + QString("%1").arg((char )bufTX.payLoad[i]);
    else
        str = str + "/" + QString("%1").arg(bufTX.payLoad[i],2,16,QChar('0')) +"/";
   }
   ui->textBrowser->setTextColor(Qt::black);
   ui->textBrowser->append("pc->mbed :"+ str);

}
