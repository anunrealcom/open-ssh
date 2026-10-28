#include "aidialog.h"

#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QRegularExpression>

AIDialog::AIDialog(QWidget *parent)
    : FramelessDialog(parent, "AI 模型配置")
{
    auto *form = new QFormLayout();
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignRight);

    m_profileCombo = new QComboBox();
    for (const AISettings &s : AIConfigStore::instance().all())
        m_profileCombo->addItem(s.name);
    m_profileCombo->addItem("＋ 新建配置");
    connect(m_profileCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AIDialog::onProfileChanged);

    m_providerCombo = new QComboBox();
    for (const AISettings &p : AIConfigStore::presets())
        m_providerCombo->addItem(p.provider);
    connect(m_providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AIDialog::onProviderChanged);

    m_name = new QLineEdit();
    m_name->setPlaceholderText("配置名，如 My-DeepSeek");
    m_baseUrl = new QLineEdit();
    m_baseUrl->setPlaceholderText("https://api.xxx.com/v1");
    m_apiKey = new QLineEdit();
    m_apiKey->setEchoMode(QLineEdit::Password);
    m_apiKey->setPlaceholderText("sk-...（本地明文保存）");
    m_model = new QLineEdit();
    m_model->setPlaceholderText("deepseek-chat");
    m_temp = new QDoubleSpinBox();
    m_temp->setRange(0.0, 2.0);
    m_temp->setSingleStep(0.1);
    m_temp->setValue(0.7);

    m_urlPreview = new QLabel();
    m_urlPreview->setStyleSheet("QLabel{color:#8a97a6;font-size:11px;}");
    m_urlPreview->setWordWrap(true);

    form->addRow("选择配置", m_profileCombo);
    form->addRow("厂商预设", m_providerCombo);
    form->addRow("配置名", m_name);
    form->addRow("API Base", m_baseUrl);
    form->addRow("API Key", m_apiKey);
    form->addRow("模型名", m_model);
    form->addRow("温度", m_temp);
    form->addRow("请求地址", m_urlPreview);
    contentLayout()->addLayout(form);

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    auto *newBtn = new QPushButton("新建");
    auto *delBtn = new QPushButton("删除");
    auto *saveBtn = new QPushButton("保存");
    auto *cancelBtn = new QPushButton("取消");
    saveBtn->setObjectName("primaryBtn");
    connect(newBtn, &QPushButton::clicked, this, &AIDialog::onNew);
    connect(delBtn, &QPushButton::clicked, this, &AIDialog::onDelete);
    connect(saveBtn, &QPushButton::clicked, this, &AIDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(newBtn);
    btnRow->addWidget(delBtn);
    btnRow->addWidget(saveBtn);
    btnRow->addWidget(cancelBtn);
    contentLayout()->addLayout(btnRow);

    // 默认载入当前配置
    AISettings cur = AIConfigStore::instance().current();
    if (cur.isValid()) {
        int idx = m_profileCombo->findText(cur.name);
        m_profileCombo->setCurrentIndex(idx >= 0 ? idx : 0);
        loadProfile(cur);
    } else {
        onProviderChanged(m_providerCombo->currentIndex());
        m_name->clear();
    }
    update(); // 触发 url 预览刷新

    // 输入变化时刷新 URL 预览
    auto refresh = [this]() {
        QString u = m_baseUrl->text().trimmed();
        if (!u.isEmpty()) {
            if (!u.endsWith("/")) u += "/";
            u += "chat/completions";
        }
        m_urlPreview->setText(u);
    };
    connect(m_baseUrl, &QLineEdit::textChanged, this, [refresh]() { refresh(); });
    refresh();
}

void AIDialog::onProfileChanged(int index)
{
    if (index < 0 || index >= AIConfigStore::instance().all().size()) {
        onNew(); // “＋ 新建配置”
        return;
    }
    loadProfile(AIConfigStore::instance().all().at(index));
}

void AIDialog::onProviderChanged(int index)
{
    QList<AISettings> ps = AIConfigStore::presets();
    if (index < 0 || index >= ps.size())
        return;
    const AISettings &p = ps.at(index);
    m_baseUrl->setText(p.baseUrl);
    m_model->setText(p.model);
    if (m_name->text().isEmpty())
        m_name->setText(p.name == "自定义" ? "" : p.name);
}

void AIDialog::onNew()
{
    m_name->clear();
    m_apiKey->clear();
    m_baseUrl->clear();
    m_model->clear();
    m_temp->setValue(0.7);
    m_profileCombo->setCurrentIndex(m_profileCombo->count() - 1); // “＋ 新建配置”
}

void AIDialog::onSave()
{
    AISettings s;
    s.name = m_name->text().trimmed();
    s.provider = m_providerCombo->currentText();
    s.baseUrl = m_baseUrl->text().trimmed();
    s.apiKey = m_apiKey->text().trimmed();
    s.model = m_model->text().trimmed();
    s.temperature = m_temp->value();

    if (s.name.isEmpty()) { m_name->setFocus(); return; }
    if (!s.baseUrl.startsWith("http")) { m_baseUrl->setFocus(); return; }
    if (s.model.isEmpty()) { m_model->setFocus(); return; }

    AIConfigStore::instance().save(s);
    AIConfigStore::instance().setCurrent(s.name);
    accept();
}

void AIDialog::onDelete()
{
    QString n = m_name->text().trimmed();
    if (n.isEmpty())
        return;
    AIConfigStore::instance().remove(n);
    m_profileCombo->clear();
    for (const AISettings &s : AIConfigStore::instance().all())
        m_profileCombo->addItem(s.name);
    m_profileCombo->addItem("＋ 新建配置");
    onNew();
}

void AIDialog::loadProfile(const AISettings &s)
{
    m_name->setText(s.name);
    m_baseUrl->setText(s.baseUrl);
    m_apiKey->setText(s.apiKey);
    m_model->setText(s.model);
    m_temp->setValue(s.temperature);
    int idx = m_providerCombo->findText(s.provider);
    if (idx >= 0)
        m_providerCombo->setCurrentIndex(idx);
}
