#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QGridLayout>
#include <QGroupBox>
#include <QMetaObject>
#include <sstream>
#include <thread>
#include <rclcpp/rclcpp.hpp>
#include <status_interface/msg/system_status.hpp>

using SystemStatus = status_interface::msg::SystemStatus;

class SysStatusDisplay : public rclcpp::Node
{
private:
    rclcpp::Subscription<SystemStatus>::SharedPtr sub_;
    QWidget *main_window_;
    QLabel *host_name_val_;
    QLabel *timestamp_val_;
    QLabel *cpu_percent_val_;
    QProgressBar *cpu_bar_;
    QLabel *memory_total_val_;
    QLabel *memory_avail_val_;
    QLabel *memory_percent_val_;
    QProgressBar *memory_bar_;
    QLabel *net_recv_val_;
    QLabel *net_sent_val_;

    void update_ui(const SystemStatus::SharedPtr msg)
    {
        host_name_val_->setText(QString::fromStdString(msg->host_name));
        timestamp_val_->setText(QString::number(msg->stamp.sec) + " s");
        
        double cpu = msg->cpu_percent;
        cpu_percent_val_->setText(QString::number(cpu, 'f', 1) + " %");
        cpu_bar_->setValue(static_cast<int>(cpu));
        
        memory_total_val_->setText(QString::number(msg->memory_total) + " MB");
        memory_avail_val_->setText(QString::number(msg->memory_available) + " MB");
        
        double mem = msg->memory_percent;
        memory_percent_val_->setText(QString::number(mem, 'f', 1) + " %");
        memory_bar_->setValue(static_cast<int>(mem));
        
        net_recv_val_->setText(QString::number(msg->net_recv) + " MB");
        net_sent_val_->setText(QString::number(msg->net_sent) + " MB");
    }

public:
    SysStatusDisplay(const std::string &node_name) : Node(node_name)
    {
        // 创建主窗口
        main_window_ = new QWidget;
        main_window_->setWindowTitle("系统资源状态监控");
        main_window_->setMinimumSize(500, 400);
        main_window_->setStyleSheet(R"(
            QWidget {
                background-color: #2e2e2e;
                font-family: "Microsoft YaHei", "SimHei";
                font-size: 12px;
                color: #f0f0f0;
            }
            QGroupBox {
                border: 1px solid #5a5a5a;
                border-radius: 5px;
                margin-top: 10px;
                font-weight: bold;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
            }
            QLabel {
                color: #dcdcdc;
            }
            QProgressBar {
                border: 1px solid #5a5a5a;
                border-radius: 3px;
                text-align: center;
                background-color: #3c3c3c;
            }
            QProgressBar::chunk {
                background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                                  stop:0 #4caf50, stop:1 #8bc34a);
                border-radius: 2px;
            }
        )");

        QGridLayout *main_layout = new QGridLayout(main_window_);

        // 基本信息
        QGroupBox *info_group = new QGroupBox("基本信息");
        QGridLayout *info_layout = new QGridLayout(info_group);
        info_layout->addWidget(new QLabel("主机名:"), 0, 0);
        host_name_val_ = new QLabel("--");
        info_layout->addWidget(host_name_val_, 0, 1);
        info_layout->addWidget(new QLabel("数据时间(s):"), 1, 0);
        timestamp_val_ = new QLabel("--");
        info_layout->addWidget(timestamp_val_, 1, 1);
        main_layout->addWidget(info_group, 0, 0, 1, 2);

        // CPU
        QGroupBox *cpu_group = new QGroupBox("CPU 使用率");
        QGridLayout *cpu_layout = new QGridLayout(cpu_group);
        cpu_layout->addWidget(new QLabel("使用率:"), 0, 0);
        cpu_percent_val_ = new QLabel("-- %");
        cpu_layout->addWidget(cpu_percent_val_, 0, 1);
        cpu_bar_ = new QProgressBar;
        cpu_bar_->setRange(0, 100);
        cpu_bar_->setFormat("%p%");
        cpu_layout->addWidget(cpu_bar_, 1, 0, 1, 2);
        main_layout->addWidget(cpu_group, 1, 0);

        // 内存
        QGroupBox *mem_group = new QGroupBox("内存信息");
        QGridLayout *mem_layout = new QGridLayout(mem_group);
        mem_layout->addWidget(new QLabel("总大小:"), 0, 0);
        memory_total_val_ = new QLabel("-- MB");
        mem_layout->addWidget(memory_total_val_, 0, 1);
        mem_layout->addWidget(new QLabel("剩余:"), 1, 0);
        memory_avail_val_ = new QLabel("-- MB");
        mem_layout->addWidget(memory_avail_val_, 1, 1);
        mem_layout->addWidget(new QLabel("使用率:"), 2, 0);
        memory_percent_val_ = new QLabel("-- %");
        mem_layout->addWidget(memory_percent_val_, 2, 1);
        memory_bar_ = new QProgressBar;
        memory_bar_->setRange(0, 100);
        memory_bar_->setFormat("%p%");
        mem_layout->addWidget(memory_bar_, 3, 0, 1, 2);
        main_layout->addWidget(mem_group, 1, 1);

        // 网络
        QGroupBox *net_group = new QGroupBox("网络流量 (累计)");
        QGridLayout *net_layout = new QGridLayout(net_group);
        net_layout->addWidget(new QLabel("接收:"), 0, 0);
        net_recv_val_ = new QLabel("-- MB");
        net_layout->addWidget(net_recv_val_, 0, 1);
        net_layout->addWidget(new QLabel("发送:"), 1, 0);
        net_sent_val_ = new QLabel("-- MB");
        net_layout->addWidget(net_sent_val_, 1, 1);
        main_layout->addWidget(net_group, 2, 0, 1, 2);

        main_window_->show();

        // 订阅话题，使用 main_window_ 作为 invokeMethod 的接收者
        sub_ = this->create_subscription<SystemStatus>(
            "sys_status", 10,
            [this](const SystemStatus::SharedPtr msg) {
                // 将 UI 更新任务投递到主线程，目标对象为 main_window_
                QMetaObject::invokeMethod(main_window_, [this, msg]() {
                    update_ui(msg);
                });
            });

        // 初始显示默认数据
        auto init_msg = std::make_shared<SystemStatus>();
        init_msg->host_name = "等待数据...";
        init_msg->stamp.sec = 0;
        init_msg->cpu_percent = 0;
        init_msg->memory_total = 0;
        init_msg->memory_available = 0;
        init_msg->memory_percent = 0;
        init_msg->net_recv = 0;
        init_msg->net_sent = 0;
        update_ui(init_msg);
    }

    ~SysStatusDisplay()
    {
        if (main_window_) delete main_window_;
    }
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    QApplication app(argc, argv);

    auto node = std::make_shared<SysStatusDisplay>("status_display");

    std::thread spin_thread([node]() {
        rclcpp::spin(node);
    });
    spin_thread.detach();

    int ret = app.exec();
    rclcpp::shutdown();
    return ret;
}