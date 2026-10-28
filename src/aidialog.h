#pragma once

#include "framelessdialog.h"
#include "ai.h"

/**
 * AI 配置对话框：管理多套 OpenAI 兼容模型的接入配置。
 * 可选厂商预设，填写 API Base / Key / 模型 / 温度后保存。
 */
class AIDialog : public FramelessDialog
{
    Q_OBJECT
public:
    explicit AIDialog(QWidget *parent = nullptr);

private slots:
    void onProfileChanged(int index);
    void onProviderChanged(int index);
    void onNew();
    void onSave();
    void onDelete();

private:
    void loadProfile(const AISettings &s);

    class QComboBox *m_profileCombo = nullptr;
    class QComboBox *m_providerCombo = nullptr;
    class QLineEdit *m_name = nullptr;
    class QLineEdit *m_baseUrl = nullptr;
    class QLineEdit *m_apiKey = nullptr;
    class QLineEdit *m_model = nullptr;
    class QDoubleSpinBox *m_temp = nullptr;
    class QLabel *m_urlPreview = nullptr;
};
