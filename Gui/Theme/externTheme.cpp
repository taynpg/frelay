#include "externTheme.h"

#include <oclero/qlementine.hpp>

QStyle* Theme::getStyle(QObject* parent)
{
    return new oclero::qlementine::QlementineStyle(parent);
}
