#pragma once

#include <QString>

namespace PlatformInput {
bool hasPermission();
void requestPermission();
// Empty means the system accepted the event; target applications may still ignore it.
QString send(const QString &unit, bool shiftEnter);
}
