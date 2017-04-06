/*
    Copyright (c) 2016 Christian Mollekofp <mollekopf@kolabsys.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/
#pragma once

#include <QAbstractItemModel>
#include <QString>

/**
 * Exposes a model and maintains a current index selection.
 */
class Selector : public QObject {
    Q_OBJECT
    Q_PROPERTY (int currentIndex READ currentIndex WRITE setCurrentIndex)
    Q_PROPERTY (QAbstractItemModel* model READ model CONSTANT)

public:
    Selector(QAbstractItemModel *model);

    virtual QAbstractItemModel *model() { return mModel; }

    void setCurrentIndex(int i) {
        mCurrentIndex = i;
        Q_ASSERT(mModel);
        if (i >= 0) {
            setCurrent(mModel->index(mCurrentIndex, 0));
        } else {
            setCurrent(QModelIndex());
        }
    }

    void reapplyCurrentIndex();

    int currentIndex() { return mCurrentIndex; }

    virtual void setCurrent(const QModelIndex &) = 0;
private:
    QAbstractItemModel *mModel = nullptr;
    int mCurrentIndex = 0;
};
