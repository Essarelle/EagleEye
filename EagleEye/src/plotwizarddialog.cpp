#include <Aquila/plotters/PlotManager.h>
#include "Aquila/plotters/Plotter.h"
#include "MetaObject/Parameters/detail/TypedParameterPtrImpl.hpp"
#include "plotwizarddialog.h"
#include "ui_plotwizarddialog.h"

#include <qsplitter.h>


PlotWizardDialog::PlotWizardDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PlotWizardDialog)
{
    ui->setupUi(this);
    // Create plots for each plotter for demonstration purposes.
    setup();
    connect(this, SIGNAL(update(int)), this, SLOT(handleUpdate(int)), Qt::QueuedConnection);
}

void PlotWizardDialog::setup()
{
    for(int i = 0; i < previewPlots.size(); ++i)
    {
        delete previewPlots[i];
    }
    previewPlots.clear();
    previewPlotters.clear();
    QSplitter* plot_splitter = new QSplitter(this);
    
    plot_splitter->setOrientation(Qt::Vertical);
    
    ui->plotPreviewLayout->addWidget(plot_splitter, 0,0);
    
    auto plotters = aq::PlotManager::Instance()->GetAvailablePlots();
    for(size_t i = 0; i < plotters.size(); ++i)
    {
        rcc::shared_ptr<aq::Plotter> plotter = aq::PlotManager::Instance()->GetPlot(plotters[i]);
        if(plotter != nullptr)
        {
            rcc::shared_ptr<aq::QtPlotter> qtPlotter(plotter);
            if(qtPlotter)
            {
                QWidget* plot = qtPlotter->CreatePlot(this);
                plot->installEventFilter(this);
                qtPlotter->AddPlot(plot);
                auto control_widget = qtPlotter->GetControlWidget(this);
                if (control_widget)
                {
                    QSplitter* setting_splitter = new QSplitter(this);
                    setting_splitter->setOrientation(Qt::Horizontal);
                    setting_splitter->addWidget(plot);
                    setting_splitter->addWidget(control_widget);
                    previewPlotControllers[plot] = control_widget;
                    plot_splitter->addWidget(setting_splitter);
                }
                else
                {
                    plot_splitter->addWidget(plot);
                }
                previewPlots.push_back(plot);
                previewPlotters.push_back(qtPlotter);
            }
        }
    }
    lastUpdateTime.resize(plotters.size());
}

PlotWizardDialog::~PlotWizardDialog()
{
    delete ui;
}
bool PlotWizardDialog::eventFilter(QObject *obj, QEvent *ev)
{
    if(ev->type() == QEvent::MouseButtonPress)
    {
        bool found = false;
        int i = 0;
        for(; i < previewPlots.size(); ++i)
        {
            if(previewPlots[i] == obj)
            {
                found = true;
                break;
            }
        }
        /*        if(found == false)
            return false;
        QMouseEvent* mev = dynamic_cast<QMouseEvent*>(ev);
        if(mev->button() == Qt::MiddleButton)
        {
            currentPlotter = previewPlotters[i];
            QDrag* drag = new QDrag(obj);
            QMimeData* data = new QMimeData();
            drag->setMimeData(data);
            drag->exec();
            return true;
        }*/
    }
    return false;
}

void PlotWizardDialog::onUpdate(int idx)
{
    if(idx >= lastUpdateTime.size())
        return;
    // Emit a signal with the idx from the current processing thread to the UI thread
    // Limit the update rate by checking update time for each idx
    boost::posix_time::ptime currentTime = boost::posix_time::microsec_clock::universal_time();
    boost::posix_time::time_duration delta = currentTime - lastUpdateTime[idx];
    // Prevent updating plots too often by limiting the update rate to every 30ms.
    if(delta.total_milliseconds() > 30)
    {
        lastUpdateTime[idx] = currentTime;
        emit update(idx);
    }
}
void PlotWizardDialog::handleUpdate(int idx)
{
    //previewPlotters[idx]->doUpdate();
}

void PlotWizardDialog::plotParameter(mo::IParameter* param)
{
    this->show();
    //ui->tabWidget->setCurrentIndex(0);
    ui->inputDataType->setText(QString::fromStdString(param->GetTypeInfo().name()));
    for(int i = 0; i < previewPlotters.size(); ++i)
    {
        if(previewPlotters[i]->AcceptsParameter(param))
        {
            previewPlotters[i]->SetInput(param);
            previewPlots[i]->show();
            previewPlots[i]->setMinimumHeight(200);
            auto itr = previewPlotControllers.find(previewPlots[i]);
            if (itr != previewPlotControllers.end())
            {
                itr->second->show();
            }
        }else
        {
            previewPlotters[i]->SetInput();
            previewPlots[i]->hide();
            auto itr = previewPlotControllers.find(previewPlots[i]);
            if (itr != previewPlotControllers.end())
            {
                itr->second->hide();
            }
        }
    }
}
void PlotWizardDialog::on_addPlotBtn_clicked()
{
    PlotWindow* plot = new PlotWindow(this);
    emit on_plotAdded(plot);
    connect(plot, SIGNAL(onDrop()), this, SLOT(on_drop()));
    QCheckBox* box = new QCheckBox(plot->getPlotName());
    box->setCheckable(true);
    box->setChecked(false);
    ui->currentPlots->addWidget(box);
    plotWindows.push_back(plot);
}
void PlotWizardDialog::on_drop()
{
    for(int i = 0; i < plotWindows.size(); ++i)
    {
        if(plotWindows[i] == sender())
        {
            if(currentPlotter != nullptr)
                plotWindows[i]->addPlotter(currentPlotter);
        }
    }
}

void PlotWizardDialog::on_tabWidget_currentChanged(int index)
{
    if(index == 2)
    {
        for(int i = 2; i < ui->currentPlots->rowCount(); ++i)
        {
            if(QCheckBox* box = dynamic_cast<QCheckBox*>(ui->currentPlots->itemAtPosition(i,0)))
                box->setText(plotWindows[i-2]->getPlotName());
        }
    }
}
