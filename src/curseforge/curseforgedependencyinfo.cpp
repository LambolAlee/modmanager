#include "curseforgedependencyinfo.h"

#include "util/tutil.hpp"
#include "curseforgefileinfo.h"

CurseforgeDependencyInfo CurseforgeDependencyInfo::fromVariant(const QVariant &variant)
{
    CurseforgeDependencyInfo info;
    info.modId_ = value(variant, "modId").toInt();
    info.type_ = static_cast<Type>(value(variant, "relationType").toInt());
    return info;
}

int CurseforgeDependencyInfo::modId() const
{
    return modId_;
}

int CurseforgeDependencyInfo::type() const
{
    return type_;
}

QString CurseforgeDependencyInfo::typeString() const
{
    switch (type_) {
    case CurseforgeDependencyInfo::EmbeddedLibrary:
        return QObject::tr("Embedded Library");
    case CurseforgeDependencyInfo::Incompatible:
        return QObject::tr("Incompatible");
    case CurseforgeDependencyInfo::OptionalDependency:
        return QObject::tr("Optional Dependency");
    case CurseforgeDependencyInfo::RequiredDependency:
        return QObject::tr("Required Dependency");
    case CurseforgeDependencyInfo::Tool:
        return QObject::tr("Tool");
    }
}
