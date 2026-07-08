#pragma once

#include <qglobal.h>
#include <qstring.h>

struct MemoGroup {
    qint64 id = 0;
    QString name;           // 分组名称
    int sortOrder = 0;      // 分组排序值，越小越靠前
    bool isDefault = false; // 是否为稳定的默认分组
    qint64 createdAt = 0;   // 创建时间戳
    qint64 updatedAt = 0;   // 最后更新时间戳
};
