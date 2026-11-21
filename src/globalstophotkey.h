#pragma once

#include <QAbstractNativeEventFilter>
#include <QObject>
#include <QString>
#include <memory>

class GlobalStopHotkey : public QObject, private QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit GlobalStopHotkey(QObject *parent = nullptr);
    ~GlobalStopHotkey() override;
    bool isRegistered() const;
    static QString shortcutText();

signals:
    void activated();

private:
    bool nativeEventFilter(const QByteArray &type, void *message, qintptr *result) override;
    struct Native;
    std::unique_ptr<Native> native_;
};
