/*
 *  Copyright (c) 2015 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_xi2_event_filter.h"

#include "kis_debug.h"
#include <QGlobalStatic>

#include <xcb/xcb.h>
#include "qxcbconnection_xi2.h"

//#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2proto.h>

#ifndef XCB_GE_GENERIC
#define XCB_GE_GENERIC 35
#endif
#ifndef XCB_GE_NOTIFY
#define XCB_ENTER_NOTIFY 7
#endif


namespace KisFakeXcb {
    typedef uint32_t Window;
    typedef uint32_t Time;

    /**
     * See a comment in QXcbConnection::xi2PrepareXIGenericDeviceEvent()
     *
     * This struct does the sam ething but without modifying the
     * source event.
     */

    typedef struct
    {
        uint8_t     type;                   /**< Always GenericEvent */
        uint8_t     extension;              /**< XI extension offset */
        uint16_t    sequenceNumber;
        uint32_t    length;                 /**< Length in 4 byte uints */
        uint16_t    evtype;
        uint16_t    deviceid;
        Time        time;
        uint32_t    detail;                 /**< Keycode or button */
        Window      root;
        Window      event;
        Window      child;
/* └──────── 32 byte boundary ────────┘ */
        uint32_t __some_weird_padding; // this padding was introduced in xcb 1.9.3
        FP1616      root_x;                 /**< Always screen coords, 16.16 fixed point */
        FP1616      root_y;
        FP1616      event_x;                /**< Always screen coords, 16.16 fixed point */
        FP1616      event_y;
        uint16_t    buttons_len;            /**< Len of button flags in 4 b units */
        uint16_t    valuators_len;          /**< Len of val. flags in 4 b units */
        uint16_t    sourceid;               /**< The source device */
        uint16_t    pad0;
        uint32_t    flags;                  /**< ::XIKeyRepeat */
        xXIModifierInfo     mods;
        xXIGroupInfo        group;
    } xXIDeviceEvent;
};

struct KisXi2EventFilter::Private
{
    QScopedPointer<QXcbConnection> connection;
};


Q_GLOBAL_STATIC(KisXi2EventFilter, s_instance)

KisXi2EventFilter::KisXi2EventFilter()
: m_d(new Private)
{
    m_d->connection.reset(new QXcbConnection(true, ":0"));
}

KisXi2EventFilter::~KisXi2EventFilter()
{
}

bool KisXi2EventFilter::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    xcb_generic_event_t *event = static_cast<xcb_generic_event_t*>(message);

    uint response_type = event->response_type & ~0x80;

    switch (response_type) {
    case XCB_GE_GENERIC: {
        xcb_ge_event_t *geEvent = reinterpret_cast<xcb_ge_event_t *>(event);

        const int eventSize = sizeof(xcb_ge_event_t) + 4 * geEvent->length;

        // Qt's code *modifies* (!) the supplied event internally!!!
        // And since we reuse Qt's code, we should make a copy of it.

        xcb_ge_event_t *copiedEvent = (xcb_ge_event_t*) malloc(eventSize);
        memcpy(copiedEvent, geEvent, eventSize);
        bool result = m_d->connection->xi2HandleEvent(copiedEvent);
        free(copiedEvent);

#if QT_VERSION >= 0x050500
        /**
         * I know we must be fed to crocodiles for the hacks like this
         * one, but is is the best thing we can do after Qt 5.5
         * started to process all the events using XInput 2.2. It
         * means that Qt is not longer subscribed to the emulated
         * mouse events that usually go after the tablet. Instead of
         * the flow which was previosly generated by X11, Qt now
         * re-emits mouse events itself in
         * QXcbConnection::xi2HandleTabletEvent() using a call to
         * QXcbWindow::handleXIMouseEvent(). And, yes, here in Krita
         * we don't have access to QXcbWindow. Even through private
         * headers. So we have to use this "elegant" solution.
         *
         * The solution is very simple. We just modify the 'sourceid'
         * field of the currently processed event and pass it further
         * to Qt. We change the source id to a random number, so Qt
         * doesn't recognize it as a tablet, scrolling or touch device
         * and therefore passes it further to the code that handles
         * mouse events.
         *
         * Now just let's hope QT will not change this behavior in the
         * future...
         */

        KisFakeXcb::xXIDeviceEvent *ev =
            reinterpret_cast<KisFakeXcb::xXIDeviceEvent*>(event);

        switch (ev->evtype) {
        case XI_ButtonPress:
        case XI_ButtonRelease:
        case XI_Motion: {
            ev->sourceid = 4815; // just a random fake source id, which Qt doesn't know about
            ev->valuators_len = 2;
            return false;
        }
        default:
            break;
        };
#endif /* QT_VERSION >= 0x050500 */

        return result;
    }
    case XCB_ENTER_NOTIFY: {
        xcb_enter_notify_event_t *enter = (xcb_enter_notify_event_t *)event;
        m_d->connection->notifyEnterEvent(enter);
    }
    default:
        break;
    }

    return false;
}

KisXi2EventFilter* KisXi2EventFilter::instance()
{
    return s_instance;
}