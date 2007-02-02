/* This file is part of the KDE project
 * Copyright (C) 2006-2007 Thomas Zander <zander@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KOTEXTDOCUMENTLAYOUT_H
#define KOTEXTDOCUMENTLAYOUT_H

#include <kotext_export.h>

#include <QAbstractTextDocumentLayout>
#include <QRectF>
#include <QSizeF>
#include <QList>

class KoTextDocumentLayout;
class KoShape;
class KoStyleManager;
class QTextLayout;
class QTextList;
class KoInlineTextObjectManager;

/**
 * KWords text layouter that allows text to flow in multiple frames and around
 * other KWord objects.
 */
class KOTEXT_EXPORT KoTextDocumentLayout : public QAbstractTextDocumentLayout {
    Q_OBJECT
public:
    class LayoutState;
    /// constructor
    explicit KoTextDocumentLayout(QTextDocument *document, KoTextDocumentLayout::LayoutState *layout = 0);
    virtual ~KoTextDocumentLayout();

    /**
     * While the text document is standalone, the text can refer to the character
     * and paragraph styles, and doing so is needed in doing proper text-layout.
     * Setting the stylemanager on this layouter is therefor required if there is one.
     */
    void setStyleManager(KoStyleManager *sm);

    void setLayout(LayoutState *layout);
    bool hasLayouter() const;

    void setInlineObjectTextManager(KoInlineTextObjectManager *iom);
    KoInlineTextObjectManager *inlineObjectTextManager();

    /// Returns the bounding rectangle of block.
    QRectF blockBoundingRect ( const QTextBlock & block ) const;
    /**
     * Returns the total size of the document. This is useful to display
     * widgets since they can use to information to update their scroll bars
     * correctly
     */
    QSizeF documentSize () const;
    /// Draws the layout on the given painter with the given context.
    void draw ( QPainter * painter, const PaintContext & context );
    /// Returns the bounding rectacle of frame. Returns the bounding rectangle of frame.
    QRectF frameBoundingRect ( QTextFrame * frame ) const;
    /**
     * Returns the cursor postion for the given point with the accuracy
     * specified. Returns -1 to indicate failure if no valid cursor position
     * was found.
     * @param point the point in the document
     * @param accuracy if Qt::ExactHit this method will return -1 when not actaully hitting any text
     */
    int hitTest ( const QPointF & point, Qt::HitTestAccuracy accuracy ) const;
    /// reimplemented to always return 1
    int pageCount () const;

    /**
     * Actually do the layout of the text.
     * This method will layout the text into lines and shapes, chunk by chunk. It will
     * return quite quick and have requested for another layout if its unfinished.
     */
    void layout();

    void interruptLayout();

    void addShape(KoShape *shape);

    virtual QList<KoShape*> shapes() const;

    KoStyleManager *styleManager() const;

    class KOTEXT_EXPORT LayoutState {
    public:
        LayoutState() : shapeNumber(-1), shape(0), layout(0) {}
        virtual ~LayoutState() {}
        /// start layouting, return false when there is nothing to do
        virtual bool start() = 0;
        /// end layouting
        virtual void end() = 0;
        virtual void reset() = 0;
        /// returns true if reset has been called.
        virtual bool interrupted() = 0;
        virtual double width() = 0;
        virtual double x() = 0;
        virtual double y() = 0;
        /// return the y offset of the document at start of shape.
        virtual double docOffsetInShape() const = 0;
        /// when a line is added, update internal vars.  Return true if line does not fit in shape
        virtual bool addLine(QTextLine &line) = 0;
        /// prepare for next paragraph; return false if there is no next parag.
        virtual bool nextParag() = 0;
        virtual double documentOffsetInShape() = 0;
        /// paint the document
        virtual void draw(QPainter *painter, const PaintContext & context ) = 0;

        int shapeNumber;
        KoShape *shape;
        QTextLayout *layout;

    protected:
        friend class KoTextDocumentLayout;
        virtual void setStyleManager(KoStyleManager *sm) = 0;
        virtual KoStyleManager *styleManager() const = 0;
    };

protected:
    LayoutState *m_state;

    friend class KoVariable;
    /// reimplemented from QAbstractTextDocumentLayout
    void documentChanged(int position, int charsRemoved, int charsAdded);

private slots:
    void relayout();

private:
    /// reimplemented
    void drawInlineObject(QPainter *painter, const QRectF &rect, QTextInlineObject object, int position, const QTextFormat &format);
    /// reimplemented
    void positionInlineObject(QTextInlineObject item, int position, const QTextFormat &format);
    /// reimplemented
    void resizeInlineObject(QTextInlineObject item, int position, const QTextFormat &format);

    virtual void scheduleLayout();

private:
    class Private;
    Private * const d;
};

#endif
