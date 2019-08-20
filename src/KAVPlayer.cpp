#include <QEvent>
#include <QDir>
#include <QDialog>
#include <QSize>
#include <QTime>
#include <QDateTime>
#include <QThread>
#include <QString>
#include <QMenu>
#include <QKeyEvent>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QColor>
#include <QPushButton>
#include <QTextEdit>
#include <QSplitter>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QUrl>
#include <QListWidget>
#include <QCoreApplication>
#include <QApplication>
#include <QDesktopWidget>
#include <cstring>
#include "KAVPlayer.h"
#include "ClickSlider.h"
#include "AVPlayerWidget.h"
#include "error/error.h"
#include "log/log.h"
#include "inifile/inifile.h"

extern "C" 
{
#include "libavutil/log.h"
}

#define FILENAME "KAVPlayer.cpp"

using namespace inifile;

const char *fileTypeStr = "Video file(*.mp4 *.avi *mkv *.mpeg *.h264 *.h265 *.mov *.wmv *.flv);;"\
                          "Music file(*.mp3 *.mp2 *.aac *.ape *.flac *.ogg *.aiff *.m4a *.wav *.wma);;"\
                          "All files(*.*)";

QRect Rect::toQRect ()
{
    return QRect(x, y, w, h);
}

void Rect::setFromQRect(QRect rect)
{
    x = rect.x();
    y = rect.y();
    w = rect.width();
    h = rect.height();
}

void KAVPlayer::errProc (int err_code)
{
    /* stop */
    stop();

    /* show error information */
    switch (err_code) {
    case KESUCCESS:
        break;
    default:
//        show_msg(, 3000);
// log
///////////////////////////////
        break;
    }
}

void KAVPlayer::stop ()
{
    /* stop */
    if (!m_videoWidget->is_stopped()) {
        m_videoWidget->stop();

        /* show message */
        m_videoWidget->show_msg("Stopped", 3000);
    }

    /* reset widgets */   
    m_infoLabel->setText(" 00:00:00.000 / 00:00:00.000");
    m_progressSlider->setSliderPosition(0);
    m_progressSlider->setDisabled(true);

    /* change icon */
    m_pause->setIcon(m_iconPlay);

    /* set window title */
    setWindowTitle("KAVPlayer");

    /* cancel fullscreen */
    QKeyEvent key_event(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QCoreApplication::sendEvent(this, &key_event);

    /* set focus */
    setFocus();
}

void KAVPlayer::priv ()
{
    if (m_playlist.isEmpty())
        return;

    /* play privious item */
    QMutableLinkedListIterator<PlaylistItem> iterator(m_playlist);
    while (iterator.hasNext()) {
        PlaylistItem next = iterator.next();
        if (iterator.peekNext().widgetItem == m_nextItem.widgetItem) {
            m_nextItem = next;

            /* close the item that is playing and play new item */
            stop();
            pause();

            /* set focus */
            setFocus();
            return;
        }    
    }

    /* play first item if no privious item */
    m_nextItem = m_playlist.first();

    /* close the item that is playing and play new item */
    stop();
    pause();

    /* set focus */
    setFocus();
}

void KAVPlayer::pause ()
{
    if (m_videoWidget->is_stopped()) { // player closed
        if (!m_nextItem.widgetItem)
            return;

        /* select current item */
        m_nextItem.widgetItem->setSelected(true);

        /* set cursor */
        this->setCursor(Qt::WaitCursor);

        /* open next file */
        int ret = m_videoWidget->open(m_nextItem.url.toStdString().c_str());
        if (ret < 0) {
            /* show message */
            QFileInfo info(m_nextItem.url);
            m_videoWidget->show_msg(("Failed to open file, Error code: " + QString::number(ret))
                                    .toStdString().c_str(), 0);
            logger.error(("Failed to open file \"" + m_nextItem.widgetItem->text() + "\" : " + getErrString(ret) + ".\n")
                                    .toStdString().c_str());

            /* set cursor */
            this->setCursor(Qt::ArrowCursor);

            /* set focus */
            setFocus();
            return;
        }
        if (m_autoFullscreen && !m_videoWidget->isFullScreen())
                m_videoWidget->switch_fullscreen(true);
        
        /* set options */
        m_videoWidget->set_volume(m_vol * 2);
        m_videoWidget->set_frame_drop(false); // unsolved bug

        /* get media info */

        /* init widgets */
        m_duration = m_videoWidget->get_duration();
        int h = (int)m_duration / 3600;
        int m = (int)m_duration % 3600 / 60;
        int s = (int)m_duration % 60;
        int ms = (int)(m_duration * 1000.0) % 1000;
        QTime t(h, m, s);
        m_strDuration = t.toString() + ".";
        m_strDuration .append(QString("%1").arg(ms,3,10,QLatin1Char('0')));
        m_infoLabel->setText(" 00:00:00.000 / " + m_strDuration);
        m_progressSlider->setSliderPosition(0);
        m_progressSlider->setDisabled(false);
        
        /* play */
        m_videoWidget->play();

        /* change icon */
        m_pause->setIcon(m_iconPause);

        /* set window title */
        setWindowTitle(m_nextItem.url);

        /* show message */
        QFileInfo info(m_nextItem.url);
        m_videoWidget->show_msg(("File " + info.fileName() + " is playing")
                                .toStdString().c_str(), 3000);

        /* set cursor */
        this->setCursor(Qt::ArrowCursor);
    } else { // player has opened a media file
        if (m_videoWidget->is_paused()) { // to play
            /* play */
            m_videoWidget->play();

            /* show message */
            m_videoWidget->show_msg("Playing...", 3000);

            /* change icon */
            m_pause->setIcon(m_iconPause);
        } else { // to pause
            /* pause */
            m_videoWidget->pause();

            /* show message */
            m_videoWidget->show_msg("Paused", 3000);

            /* change icon */
            m_pause->setIcon(m_iconPlay);
        }
    }

    /* set focus */
    setFocus();
}

void KAVPlayer::next ()
{
    /* play next */
    playNextListItem();
}

void KAVPlayer::switchList ()
{
    QResizeEvent e(size(), size());

    m_showList = !m_showList;
    if (m_showList) { // to show
        m_playerPane->setGeometry(0, 0, m_windowRect.w - m_listPaneRect.w, m_playerPaneRect.h);
        m_listPane->setGeometry(0, 0, m_listPaneRect.w, m_listPaneRect.h);
        m_listPane->show();
        m_settingMenu->findChild<QAction *>("0-0")->setIcon(QIcon(m_iconYes));;
        QCoreApplication::sendEvent(this, &e);
    } else { // to hide
        m_playerPane->setGeometry(0, 0, m_windowRect.w, m_playerPaneRect.h);
        m_listPane->hide();
        m_settingMenu->findChild<QAction *>("0-0")->setIcon(QIcon(m_iconNo));;
        QCoreApplication::sendEvent(this, &e);
    }
    setMinimumWidth(m_showList ? MIN_WINDOW_W : MIN_WINDOW_W_NOLIST);

    /* set focus */
    setFocus();
}

void KAVPlayer::setVolume (int value)
{
    value = min(max(value, 0), 64);
    m_videoWidget->set_volume(value * 2);
    m_vol = value;
    if (m_vol) {
        m_mute->setIcon(m_iconMaxVol);
        m_videoWidget->show_msg(("Volume: " + QString::number((int)((((double)m_vol / 64.0)) * 100)) + "%")
                 .toStdString().c_str(), 3000);
        m_volumeSlider->setSliderPosition(m_vol);

    } else {
        m_mute->setIcon(m_iconMute);
        m_videoWidget->show_msg("Muted", 3000);
    }

    /* set focus */
    setFocus();
}

void KAVPlayer::switchMute ()
{
    if (m_vol) {
        m_oldVol = m_vol;
        m_vol = 0;
        setVolume(m_vol);
    } else {
        m_vol = m_oldVol;
        m_oldVol = m_vol;
        setVolume(m_vol);
    }
}

void KAVPlayer::seek ()
{
    /* set cursor */
    this->setCursor(Qt::WaitCursor);

    /* seek */
    double pos = ((double)m_progressSlider->sliderPosition() / (double)m_progressSlider->maximum() * m_duration);
    if (!m_videoWidget->is_stopped()) {
        if (m_progressSlider->sliderPosition() == m_progressSlider->maximum()) {
            playNextListItem();
        } else {
            int h = (int)pos / 3600;
            int m = (int)pos / 60;
            int s = (int)pos % 60;
            int ms = (int)(pos * 1000.0) % 1000;
            QTime t(h, m, s);
            m_videoWidget->show_msg(("Seeking to " + t.toString() + "." + QString("%1")
                                    .arg(ms, 3, 10, QLatin1Char('0')))
                     .toStdString().c_str() , 3000);
            int ret = m_videoWidget->seek(pos);
            if (ret) {
                QFileInfo info(m_nextItem.url);
                m_videoWidget->show_msg(("Seeking failure, Error code: " + QString::number(ret))
                                        .toStdString().c_str(), 3000);

                /* set cursor */
                this->setCursor(Qt::ArrowCursor);
                return;
            }
        }
    }
    m_stopUpdateProgressPos = false;

    /* set cursor */
    this->setCursor(Qt::ArrowCursor);

    /* set focus */
    setFocus();
}

void KAVPlayer::updatePorgressPos (double pos)
{
    m_currentPos = pos;
    if (!m_stopUpdateProgressPos)
        m_progressSlider->setSliderPosition((int)(m_currentPos / m_duration * (double)m_progressSlider->maximum()));

    int h = (int)pos / 3600;
    int m = (int)pos / 60;
    int s = (int)pos % 60;
    int ms = (int)(pos * 1000.0) % 1000;
    QTime t(h, m, s);
    QString strCurPos(" ");
    strCurPos.append(t.toString() + ".");
    strCurPos.append(QString("%1").arg(ms, 3, 10, QLatin1Char('0'))); 
    m_infoLabel->setText(strCurPos + " / " + m_strDuration);
}

void KAVPlayer::stopUpdateProgressPos ()
{
    m_stopUpdateProgressPos = true;
}

void KAVPlayer::selListItem (QListWidgetItem * item)
{
    QMutableLinkedListIterator<PlaylistItem> iterator(m_playlist);
    while (iterator.hasNext()) {
        PlaylistItem next = iterator.next();
        if (next.widgetItem == item) {
           m_nextItem = next;
           return;
        }
    }
    m_nextItem.widgetItem = NULL;

    /* set focus */
    setFocus();
}

void KAVPlayer::playListItem (QListWidgetItem * item)
{
    QMutableLinkedListIterator<PlaylistItem> iterator(m_playlist);
    while (iterator.hasNext()) {
        PlaylistItem next = iterator.next();
        if (next.widgetItem == item) {
            m_nextItem = next;

            /* close the media file that is playing and play new media file */
            stop();
            pause();

            /* set focus */
            setFocus();
            return;
        }
    }
}

void KAVPlayer::openFile ()
{
    /* select a file */
    QString url = QFileDialog::getOpenFileName(this, "Open Media file", 
                                               (m_lastOpenedPath.isEmpty()) ? QDir::homePath() : m_lastOpenedPath, 
                                               fileTypeStr); 
    if (url.isEmpty())
        return;
    QFileInfo lastOpenedFile(url);
    m_lastOpenedPath = lastOpenedFile.path();
    QFileInfo info = QFileInfo(url);
    PlaylistItem item;
    item.url = url;

    /* check for duplicates */
    QMutableLinkedListIterator<PlaylistItem> iterator(m_playlist);
    while (iterator.hasNext()) {
        PlaylistItem item = iterator.next();
        if (url == item.url)
            goto play;
    }

    /* add net item to list widget */
    item.widgetItem = new(std::nothrow) QListWidgetItem;
    item.widgetItem->setText(info.fileName());
    item.widgetItem->setToolTip(item.url);
    item.widgetItem->setBackgroundColor(QColor(51, 51, 51));
    item.widgetItem->setIcon(m_iconVideo);
    m_playlistWidget->addItem(item.widgetItem);
    m_nextItem = item;

    /* add new item to playlist */
    m_playlist.append(item);
    item.widgetItem->setSelected(true);

play:
    /* close the media file that is playing and play new media file */
    stop();
    pause();

    /* set focus */
    setFocus();
}

void KAVPlayer::openUrl ()
{
    /* input URL */
    bool ok;
    QString url = QInputDialog::getText(nullptr, "Input URL", "Please input URL", 
                                        QLineEdit::Normal, "", &ok, Qt::MSWindowsFixedSizeDialogHint);
    if (!ok && url.isEmpty())
        return;
    QFileInfo lastOpenedFile(url);
    m_lastOpenedPath = lastOpenedFile.path();
    QFileInfo info = QFileInfo(url);
    PlaylistItem item;
    item.url = url;

    bool match = QRegExp("([A-Za-z]{3,9}://[-A-Za-z0-9+&@#/%?=~_|!:,.;]+[-A-Za-z0-9+&@#/%=~_|])")
                 .exactMatch(item.url);
    if (!match) {
        m_videoWidget->show_msg("Invalid Url", 3000);
        return;
    }

    /* check for duplicates */
    QMutableLinkedListIterator<PlaylistItem> iterator(m_playlist);
    while (iterator.hasNext()) {
        PlaylistItem next = iterator.next();
        if (url == next.url) {
            m_nextItem = item;
            goto play;
        }
    }

    /* add net item to list widget */
    item.widgetItem = new(std::nothrow) QListWidgetItem;
    if (!item.widgetItem)
        QApplication::exit(KENOMEM);
    item.widgetItem->setText(match ? item.url : info.fileName());
    item.widgetItem->setToolTip(item.url);
    item.widgetItem->setBackgroundColor(QColor(51, 51, 51));
    item.widgetItem->setIcon(m_iconVideo);
    m_playlistWidget->addItem(item.widgetItem);
    m_nextItem = item;

    /* add new item to playlist */
    m_playlist.append(item);
    item.widgetItem->setSelected(true);

play:
    /* close the media file that is playing and play new media file */
    stop();
    pause();

    /* set focus */
    setFocus();
}

void KAVPlayer::deleteItem ()
{
    /* delete item selected */
    if (m_nextItem.widgetItem) {
        QMutableLinkedListIterator<PlaylistItem> iterator(m_playlist);
        while (iterator.hasNext()) {
            PlaylistItem &item = iterator.next();
            if (item.widgetItem == m_nextItem.widgetItem) {
                iterator.remove();
                m_playlistWidget->removeItemWidget(m_nextItem.widgetItem);
                delete m_nextItem.widgetItem;
                m_nextItem.widgetItem = NULL;
                break;
            }
        }
    }

    /* set focus */
    setFocus();
}

void KAVPlayer::clearList ()
{
    /* stop */
    stop();

    /* clear list */
    m_playlist.clear();
    m_playlistWidget->clear();
    m_nextItem.widgetItem = NULL;

    /* set focus */
    setFocus();
}

void KAVPlayer::playNextListItem ()
{
    /* close the item that is playing */
    stop();

    /* select next item */
    selNextListItem();

    /* play new item */
    pause();

    /* set focus */
    setFocus();
}

void KAVPlayer::nextPlayMode ()
{
    m_playMode = ++m_playMode > PLAY_MODE_SINGLE ? PLAY_MODE_LIST_SEQUENCE : m_playMode;

    /* change icon */
    switch (m_playMode) {
    case PLAY_MODE_LIST_SEQUENCE:
        m_playModeSwitch->setIcon(m_iconList);
        break;
    case PLAY_MODE_LIST_CYCLE:
        m_playModeSwitch->setIcon(m_iconListCycle);
        break;
    case PLAY_MODE_SIGNAL_CYCLE:
        m_playModeSwitch->setIcon(m_iconSingleCycle);
        break;
    case PLAY_MODE_RANDOM:
        m_playModeSwitch->setIcon(m_iconRadom);
        break;
    case PLAY_MODE_SINGLE:
        m_playModeSwitch->setIcon(m_iconSingle);
        break;
    }

    /* set focus */
    setFocus();
}

void KAVPlayer::changeListWidth (int pos)
{
    QResizeEvent e(size(), size());
    QCoreApplication::sendEvent(this, &e);
}

void KAVPlayer::switchTopMost ()
{
    if (!m_topMost) { // switch to top most
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
        show();
        m_settingMenu->findChild<QAction *>("0-1")->setIcon(QIcon(m_iconYes));;
    } else { // cancel top most
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
        show();
        m_settingMenu->findChild<QAction *>("0-1")->setIcon(QIcon(m_iconNo));;
    }
    m_topMost = !m_topMost;
    if (m_videoWidget->is_paused())
        m_videoWidget->update_video(); 
}

void KAVPlayer::switchAutoFullscr ()
{
    if (!m_autoFullscreen) { // switch to auto fullscreen
        m_settingMenu->findChild<QAction *>("0-2")->setIcon(QIcon(m_iconYes));;
    } else { // cancel auto fullscreen
        m_settingMenu->findChild<QAction *>("0-2")->setIcon(QIcon(m_iconNo));;
    }
    m_autoFullscreen = !m_autoFullscreen;
}

void KAVPlayer::switchSavePos ()
{
    if (!m_savePos) { // save pos when closed
        m_settingMenu->findChild<QAction *>("0-3")->setIcon(QIcon(m_iconYes));;
    } else { // don't save pos when closed
        m_settingMenu->findChild<QAction *>("0-3")->setIcon(QIcon(m_iconNo));;
    }
    m_savePos = !m_savePos;
}

void KAVPlayer::switchSaveSize ()
{
    if (!m_saveSize) { // switch to auto fullscreen
        m_settingMenu->findChild<QAction *>("0-4")->setIcon(QIcon(m_iconYes));;
    } else { // cancel auto fullscreen
        m_settingMenu->findChild<QAction *>("0-4")->setIcon(QIcon(m_iconNo));;
    }
    m_saveSize = !m_saveSize;
}

void KAVPlayer::switchAutoCleanList ()
{
    if (!m_autoCleanList) { // switch auto clean list when closed
        m_settingMenu->findChild<QAction *>("1-0")->setIcon(QIcon(m_iconYes));;
    } else { // cancel auto clean list when closed 
        m_settingMenu->findChild<QAction *>("1-0")->setIcon(QIcon(m_iconNo));;
    }
    m_autoCleanList = !m_autoCleanList ;
}

void KAVPlayer::switchToExactSeekMode ()
{
    QMenu *menu = m_settingMenu->findChild<QMenu *>("2-0");
    menu->findChild<QAction *>("2-0-0")->setIcon(m_iconYes);
    menu->findChild<QAction *>("2-0-1")->setIcon(m_iconNo);
    m_fastSeek = false;
}

void KAVPlayer::switchToFastSeekMode ()
{
    QMenu *menu = m_settingMenu->findChild<QMenu *>("2-0");
    menu->findChild<QAction *>("2-0-0")->setIcon(m_iconNo);
    menu->findChild<QAction *>("2-0-1")->setIcon(m_iconYes);
    m_fastSeek = true;
}

void KAVPlayer::step ()
{
    if (!m_videoWidget->is_stopped() && m_videoWidget->is_paused()) {
        m_videoWidget->show_msg("Step forward a frame", 1000);
        m_videoWidget->step();
    }
}

void KAVPlayer::showMediaFileInfo ()
{
}

void KAVPlayer::switchHWAcce ()
{
    if (!m_hwAcce) { // switch to hardware acceleration mode
        m_settingMenu->findChild<QAction *>("3-0")->setIcon(m_iconYes);
    } else { // cancel switch hardware acceleration mode
        m_settingMenu->findChild<QAction *>("3-0")->setIcon(m_iconNo);
    }
    m_hwAcce = !m_hwAcce;
}

void KAVPlayer::resizeEvent (QResizeEvent * e)
{
    QMainWindow::resizeEvent(e);

    /* set window rect */
    m_windowRect.setFromQRect(geometry());
    m_splitter->setGeometry(0, 0, m_windowRect.w, m_windowRect.h);

    /* set player pane rect */
    m_playerPaneRect.setFromQRect(m_playerPane->geometry());

    /* set list pane rect */
    m_listPaneRect.setFromQRect(m_listPane->geometry());

    /* set video rect */
    m_videoRect.w = m_playerPaneRect.w;
    m_videoRect.h = m_playerPaneRect.h - BTN_W - PROGRESS_BAR_H - 2 * DIV_LINE_W;

    /* set progress bar rect */
    m_progressPaneRect.y = m_videoRect.h + DIV_LINE_W;
    m_progressPaneRect.w = m_playerPaneRect.w;

    /* set control bar rect */
    m_controlBarRect.y = m_videoRect.h + m_progressPaneRect.h + 2 * DIV_LINE_W;
    m_controlBarRect.w = m_playerPaneRect.w;

    /* set geometry */
    m_splitter->setGeometry(0, 0, m_windowRect.w, m_windowRect.h);
    m_progressPane->setGeometry(m_progressPaneRect.toQRect());
    m_controlBar->setGeometry(m_controlBarRect.toQRect());
    m_videoWidget->set_size(m_videoRect.w, m_videoRect.h);
    m_msgLabel->setGeometry(0, 0, m_videoWidget->width(), MSG_LABEL_H);
    if (m_windowRect.h - BTN_W - PROGRESS_BAR_H < m_msgLabel->height())
        m_msgLabel->hide();
    else
        m_videoWidget->show_msg(("" + QString::number(m_videoRect.w) + " x "
                                 + QString::number(m_videoRect.h))
                                .toStdString().c_str(), 3000);
    m_stop->setGeometry(0, 0, BTN_W, BTN_W);
    m_priv->setGeometry(BTN_W + DIV_LINE_W, 0, 
                        BTN_W, BTN_W);
    m_pause->setGeometry(2 * BTN_W + 2 * DIV_LINE_W,
                         0, BTN_W, BTN_W);
    m_next->setGeometry(3 * BTN_W + 3 * DIV_LINE_W,
                        0, BTN_W, BTN_W);
    m_listSwitch->setGeometry(m_controlBarRect.w - BTN_W,
                              0, BTN_W, BTN_W);
    m_infoLabel->setGeometry(4 * BTN_W + 4 * DIV_LINE_W, 0, 
                             m_controlBarRect.w - 5 * BTN_W - 5 * DIV_LINE_W, BTN_W);
#ifdef __MACOSX__
    m_progressSlider->setGeometry(PROGRESS_BAR_H / 4, 0,
                                  m_playerPaneRect.w - VOLUME_BAR_W - 2 * PROGRESS_BAR_H, PROGRESS_BAR_H);
#else
    m_progressSlider->setGeometry(PROGRESS_BAR_H / 4, PROGRESS_BAR_H / 4,
                                  m_playerPaneRect.w - VOLUME_BAR_W - 2 * PROGRESS_BAR_H, PROGRESS_BAR_H / 2);
#endif
    m_mute->setGeometry(m_progressSlider->width() + PROGRESS_BAR_H / 2, 0,
                        PROGRESS_BAR_H, PROGRESS_BAR_H);
#ifdef __MACOSX__
    m_volumeSlider->setGeometry(m_progressPane->width() - PROGRESS_BAR_H / 4 - VOLUME_BAR_W, 0,
                                VOLUME_BAR_W, PROGRESS_BAR_H);
#else
    m_volumeSlider->setGeometry(m_progressPane->width() - PROGRESS_BAR_H / 4 - VOLUME_BAR_W, PROGRESS_BAR_H / 4,
                                VOLUME_BAR_W, PROGRESS_BAR_H / 2);
#endif
    m_playlistWidget->setGeometry(0, 0, m_listPaneRect.w,
                                  m_listPaneRect.h - BTN_W - DIV_LINE_W);
    m_open->setGeometry(BTN1_DIV_W, m_listPaneRect.h - BTN_W * 5 / 6,
                        BTN1_W, BTN1_W);
    m_delete->setGeometry(2 * BTN1_DIV_W + BTN1_W, m_listPaneRect.h - BTN_W * 5 / 6, 
                           BTN1_W, BTN1_W);
    m_playModeSwitch->setGeometry(3 * BTN1_DIV_W + 2 * BTN1_W, m_listPaneRect.h - BTN_W * 5 / 6, 
                                  BTN1_W, BTN1_W);
    m_clear->setGeometry(4 * BTN1_DIV_W + 3 * BTN1_W, m_listPaneRect.h - BTN_W * 5 / 6,
                          BTN1_W, BTN1_W);
    m_setting->setGeometry(5 * BTN1_DIV_W + 4 * BTN1_W, m_listPaneRect.h - BTN_W * 5 / 6, 
                          BTN1_W, BTN1_W);
//    m_setting->setGeometry(5 * BTN1_DIV_W + 4 * BTN1_W, m_listPaneRect.h - BTN_W * 5 / 6, 
//                          m_listPaneRect.w - 4 * BTN1_W - 6 * BTN1_DIV_W, BTN1_W);

    /* set progress slider */
    m_progressSlider->setRange(0, m_progressSlider->width());
    if (!m_videoWidget->is_stopped()) {
        m_progressSlider->setSliderPosition((int)(m_currentPos / m_duration * (double)m_progressSlider->maximum()));
    }

    setFocus();
}

void KAVPlayer::changeEvent (QEvent * e)
{
    if (e->type() != QEvent::WindowStateChange) 
        return;

    /* resize, to fix win7 */
    if (windowState() == Qt::WindowMaximized || windowState() == Qt::WindowNoState) {
        QSize newSize = geometry().size();
        QResizeEvent e(newSize, size());
        QCoreApplication::sendEvent(this, &e);
    } else {
        QMainWindow::changeEvent(e);
    }

    setFocus();
}

void KAVPlayer::mousePressEvent (QMouseEvent * e)
{
}

void KAVPlayer::mouseReleaseEvent (QMouseEvent * e)
{
}

void KAVPlayer::mouseMoveEvent (QMouseEvent * e)
{
}

void KAVPlayer::keyPressEvent (QKeyEvent * e)
{
    int pos;
    int ret;

    if (!m_videoWidget)
        return;

    switch (e->key()) {
    case Qt::Key_S:
        step();
        break;
    case Qt::Key_P:
    case Qt::Key_Space:
        pause();
        break;
    case Qt::Key_Q:
        stop(); 
    case Qt::Key_Escape:
        if (m_videoWidget->isFullScreen())
            m_videoWidget->switch_fullscreen(false);
        m_videoWidget->show_msg(("" + QString::number(m_videoWidget->width()) + " x "
                                 + QString::number(m_videoWidget->height()))
                                .toStdString().c_str(), 3000);
        break;
    case Qt::Key_F:
        if (m_videoWidget->isFullScreen()) {
            m_videoWidget->switch_fullscreen(false);
            m_videoWidget->show_msg(("" + QString::number(m_videoWidget->width()) + " x "
                                     + QString::number(m_videoWidget->height()))
                                    .toStdString().c_str(), 3000);
        } else{
            m_videoWidget->switch_fullscreen(true);
            m_videoWidget->show_msg(("Fullscreen " + QString::number(m_videoWidget->width()) + " x "
                                     + QString::number(m_videoWidget->height()))
                                    .toStdString().c_str(), 3000);
        }
        break;
    case Qt::Key_M:
        switchMute();
        break;
    case Qt::Key_Up:
        setVolume(m_vol + 1);
        break;
    case Qt::Key_Down:
        setVolume(m_vol - 1);
        break;
    case Qt::Key_Home:
        pos = 0.0;
        goto to_seek;
    case Qt::Key_Left:
        pos = m_videoWidget->get_pos() - 1.0;
        goto to_seek;
    case Qt::Key_Right:
        pos = m_videoWidget->get_pos() + 1.0;
        goto to_seek;
    case Qt::Key_PageUp:
        pos = m_videoWidget->get_pos() - 10.0;
        goto to_seek;
    case Qt::Key_PageDown:
        pos = m_videoWidget->get_pos() + 10.0;
to_seek:
        /* set cursor */
        this->setCursor(Qt::WaitCursor);

        if (!m_videoWidget->is_stopped()) {
            if (m_videoWidget->get_duration() - pos < 0.001) {
                playNextListItem();
            } else {
                if (pos < 0.0)
                    pos = 0.0;
                int h = (int)pos / 3600;
                int m = (int)pos / 60;
                int s = (int)pos % 60;
                int ms = (int)(pos * 1000.0) % 1000;
                QTime t(h, m, s);
                m_videoWidget->show_msg(("Seeking to " + t.toString() + "." + QString("%1")
                                        .arg(ms, 3, 10, QLatin1Char('0')))
                         .toStdString().c_str() , 3000);
                int ret = m_videoWidget->seek(pos);
                if (ret) {
                    QFileInfo info(m_nextItem.url);
                    m_videoWidget->show_msg(("Seeking failure, Error code: " + QString::number(ret))
                                            .toStdString().c_str(), 3000);

                    /* set cursor */
                    this->setCursor(Qt::ArrowCursor);
                    return;
                }
            }
        }
        m_stopUpdateProgressPos = false;

        /* set cursor */
        this->setCursor(Qt::ArrowCursor);
        break;
    default:
        QMainWindow::keyPressEvent(e);
    }

    /* set focus */
    setFocus();
}

void KAVPlayer::closeEvent (QCloseEvent * e)
{
    /* stop */
    m_videoWidget->stop();

    /* save setting */
    saveSetting();

    /* save playlist*/
    savePlaylist();

    QMainWindow::closeEvent(e);
}

void KAVPlayer::selNextListItem ()
{
    QMutableLinkedListIterator<PlaylistItem> iterator(m_playlist);
    QListWidgetItem *                        sleectedItem = NULL;
    int                                      i;

    if (m_playlist.isEmpty())
        return;

    switch (m_playMode) {
    case PLAY_MODE_LIST_SEQUENCE:
        while (iterator.hasNext()) {
            PlaylistItem next = iterator.next();
            if (next.widgetItem == m_nextItem.widgetItem) {
                if (iterator.hasNext()) {
                    next = iterator.next();
                    m_nextItem = next;
                    return;
                }
            }
        }
        m_nextItem.widgetItem = NULL;
        sleectedItem = (m_playlistWidget->selectedItems().isEmpty() ? NULL : m_playlistWidget->selectedItems().first());
        if (sleectedItem)
            sleectedItem->setSelected(false);
        break;
    case PLAY_MODE_LIST_CYCLE:
        while (iterator.hasNext()) {
            PlaylistItem next = iterator.next();
            if (next.widgetItem == m_nextItem.widgetItem) {
                if (iterator.hasNext()) {
                    next = iterator.next();
                    m_nextItem = next;
                    return;
                }
                break;
            }
        }
        m_nextItem = m_playlist.first();
        break;
    case PLAY_MODE_SIGNAL_CYCLE:
        break;
    case PLAY_MODE_RANDOM:
        m_nextItem.widgetItem = NULL;
        if (!m_playlist.size())
            break;
        srand(time(NULL));
        i = abs(rand() % m_playlist.size());
        while (i >= 0) {
            PlaylistItem next = iterator.next();
            if (!i) {
                if (next.widgetItem == m_nextItem.widgetItem)
                    m_nextItem = m_playlist.first();
                else 
                    m_nextItem = next;
            }
            i--;
        }
        break;
    case PLAY_MODE_SINGLE:
        m_nextItem.widgetItem = NULL;
        break;
    }
}

void KAVPlayer::loadSetting ()
{ 
    int       ret;
    int       tempInt;
    QString   tempString;
    IniFile   loader;
    FILE *    fp = NULL;
    QRect     first_scr_rect = QApplication::desktop()->screenGeometry(0);

    /* load window setting from setting file */
    QString path = m_appDirPath + "/setting/";
    QString fileName = path + "setting.ini";
    ret = loader.load(fileName.toLocal8Bit().toStdString());

    /* load setting version */
    tempString = QString::fromStdString(loader.getStringValue("SETTING_VERSION", "VER_ID", ret));
    if (ret < 0 || tempString != VERSION)
        loader.clear();

    /* laod show list flag */
    tempInt = loader.getIntValue("WINDOW_STATUS", "SHOW_LIST", ret);
    m_showList = ret < 0 ? false : !!tempInt;

    /* load top most flag */
    tempInt = loader.getIntValue("WINDOW_STATUS", "TOP_MOST", ret); 
    m_topMost = ret < 0 ? false : !!tempInt;

    /* load top most flag */
    tempInt = loader.getIntValue("WINDOW_STATUS", "AUTO_FULLSCREEN", ret); 
    m_autoFullscreen = ret < 0 ? false : !!tempInt;

    /* load save pos flag */
    tempInt = loader.getIntValue("WINDOW_STATUS", "SAVE_POS", ret); 
    m_savePos = ret < 0 ? false : !!tempInt;

    /* load save size flag */
    tempInt = loader.getIntValue("WINDOW_STATUS", "SAVE_SIZE", ret); 
    m_saveSize = ret < 0 ? false : !!tempInt;

    /* load auto clean list flag */
    tempInt = loader.getIntValue("LIST", "AUTO_CLEAN_LIST", ret); 
    m_autoCleanList = ret < 0 ? false : !!tempInt;

    /* load play mode */
    tempInt = loader.getIntValue("LIST", "PLAY_MODE", ret); 
    m_playMode = ret < 0 ? 0 : max((int)PLAY_MODE_LIST_SEQUENCE, min((int)PLAY_MODE_SINGLE, tempInt));

    /* load volume */
    tempInt = loader.getIntValue("PLAYER_STATUS", "VOLUME", ret);
    m_vol = ret < 0 ? MAX_VOL : max(0, min(MAX_VOL, tempInt));

    /* load seek mode */
    tempInt = loader.getIntValue("PLAYER_STATUS", "FAST_SEEK", ret); 
    m_fastSeek = ret < 0 ? false : !!tempInt;

    /* load hardware acceleration flag */
    tempInt = loader.getIntValue("PLAYER_STATUS", "HW_ACCE", ret); 
    m_hwAcce = ret < 0 ? false : !!tempInt;

    /* load window rect */
    tempInt = loader.getIntValue("WINDOW_RECT", "W", ret); 
    m_windowRect.w = max(m_showList ? MIN_WINDOW_W : MIN_WINDOW_W_NOLIST, ret < 0 ? DEF_WINDOW_W: tempInt);
    tempInt = loader.getIntValue("WINDOW_RECT", "H", ret);
    m_windowRect.h = max(MIN_WINDOW_H, ret < 0 ? DEF_WINDOW_H: tempInt);
    tempInt = loader.getIntValue("WINDOW_RECT", "X", ret); 
    m_windowRect.x = max(0, ret < 0 ? (first_scr_rect.width() / 2 - m_windowRect.w / 2) : tempInt);
    tempInt = loader.getIntValue("WINDOW_RECT", "Y", ret); 
    m_windowRect.y = max(0, ret < 0 ? (first_scr_rect.height() / 2 - m_windowRect.h / 2) : tempInt);

    /* load list pane rect */
    tempInt = loader.getIntValue("LIST_PANE_RECT", "W", ret);
    m_listPaneRect.w = ret < 0 ? MIN_LIST_W: max(MIN_LIST_W, min(m_windowRect.w - MIN_WINDOW_W_NOLIST - FRAME_W, tempInt));
    m_listPaneRect.h = m_windowRect.h;
    m_listPaneRect.x = 0;
    m_listPaneRect.y = 0;

    /* load player pane rect */
    m_playerPaneRect.x = 0;
    m_playerPaneRect.y = 0;
    m_playerPaneRect.w = (m_showList ? (m_windowRect.w  - m_listPaneRect.w - FRAME_W) : (m_windowRect.w));
    m_playerPaneRect.h = m_windowRect.h;

    /* load video rect */
    m_videoRect.x = 0;
    m_videoRect.y = 0;
    m_videoRect.w = m_playerPaneRect.w;
    m_videoRect.h = m_playerPaneRect.h - BTN_W - PROGRESS_BAR_H - 2 * DIV_LINE_W;

    /* load progress bar rect */
    m_progressPaneRect.x = 0;
    m_progressPaneRect.y = m_videoRect.h + DIV_LINE_W;
    m_progressPaneRect.w = m_playerPaneRect.w;
    m_progressPaneRect.h = PROGRESS_BAR_H;
    
    /* laod control bar rect */
    m_controlBarRect.x = 0;
    m_controlBarRect.y = m_videoRect.h + m_progressPaneRect.h + 2 * DIV_LINE_W;
    m_controlBarRect.w = m_playerPaneRect.w;
    m_controlBarRect.h = BTN_W;
}

void KAVPlayer::saveSetting()
{
    /* make setting directory and setting file */
    QString path = m_appDirPath + "/setting/";
    QString fileName = path + "setting.ini";
    QFileInfo file(fileName);
    QDir dir(path);
    if (!dir.exists(path) && !dir.mkpath(path))
        return;
    if (!file.isFile()) {
        FILE *fp = fopen(fileName.toLocal8Bit(), "w+"); // using QString::toLocal8Bit() when input to fopen()
        if (!fp)
            return;
        fclose(fp);
        fp = NULL;
    }

    IniFile saver;
    /* save version */
    saver.setValue("SETTING_VERSION", "VER_ID", VERSION);
    if (m_savePos) {
        saver.setValue("WINDOW_RECT", "X", std::to_string(geometry().x()));
        saver.setValue("WINDOW_RECT", "Y", std::to_string(geometry().y()));
    }
    if (m_saveSize) {
        saver.setValue("WINDOW_RECT", "W", std::to_string(m_windowRect.w));
        saver.setValue("WINDOW_RECT", "H", std::to_string(m_windowRect.h));
    }
    saver.setValue("LIST_PANE_RECT", "W", std::to_string(m_listPaneRect.w));
    saver.setValue("WINDOW_STATUS", "SHOW_LIST", m_showList ? "1" : "0");
    saver.setValue("WINDOW_STATUS", "TOP_MOST", m_topMost ? "1" : "0");
    saver.setValue("WINDOW_STATUS", "SAVE_POS", m_savePos ? "1" : "0");
    saver.setValue("WINDOW_STATUS", "SAVE_SIZE", m_saveSize ? "1" : "0");
    saver.setValue("WINDOW_STATUS", "AUTO_FULLSCREEN", m_autoFullscreen ? "1" : "0");
    saver.setValue("LIST", "AUTO_CLEAN_LIST", m_autoCleanList ? "1" : "0");
    saver.setValue("LIST", "PLAY_MODE", std::to_string(m_playMode));
    saver.setValue("PLAYER_STATUS", "VOLUME", std::to_string(m_vol));
    saver.setValue("PLAYER_STATUS", "FAST_SEEK", std::to_string(m_fastSeek));
    saver.setValue("PLAYER_STATUS", "HW_ACCE", m_hwAcce ? "1" : "0");
    saver.saveas(fileName.toLocal8Bit().toStdString());
}

void KAVPlayer::loadPlaylist ()
{
    QMutableLinkedListIterator<PlaylistItem> iterator(m_playlist);
    QListWidgetItem *                        widget;
    QString                                  tempString;
    IniFile                                  loader;
    QFileInfo                                info;
    QString                                  key;
    FILE *                                   fp = NULL;
    int                                      ret;
    int                                      tempInt;
    double                                   tempDouble;

    /* load window playlist from playlist file */
    QString path = m_appDirPath + "/playlist/";
    QString fileName = path + "playlist.pl";
    ret = loader.load(fileName.toLocal8Bit().toStdString());
    if (ret < 0)
        return;

    /* get setting version */
    tempString = QString::fromStdString(loader.getStringValue("PLAYLIST_VERSION", "VER_ID", ret));
    if (ret < 0 || tempString != VERSION)
        loader.clear();

    /* load play history */
    m_nextItem.url = QString::fromStdString(loader.getStringValue("PLAY_HISTORY", "URL", ret));
    m_nextItem.widgetItem = NULL;
    if (!QRegExp("([A-Za-z]{3,9}://[-A-Za-z0-9+&@#/%?=~_|!:,.;]+[-A-Za-z0-9+&@#/%=~_|])")
         .exactMatch(m_nextItem.url)) {
        QFileInfo fileInfo(m_nextItem.url);
        if (!fileInfo.exists()) 
            m_nextItem.url.clear();
    }


    /* load playlist */
    tempInt = loader.getIntValue("PLAY_LIST", "LIST_LEN", ret);
    for (int i = 0; i < tempInt; i++) {
        /* read a playlist item */
        PlaylistItem item;
        key = "URL" + QString::number(i);
        item.url = QString::fromStdString(loader.getStringValue("PLAY_LIST", key.toStdString(), ret));
        if (item.url.isEmpty() || ret < 0)
            break;

        /* if the file does not exist and is not a URL, throw it */
        bool match = QRegExp("([A-Za-z]{3,9}://[-A-Za-z0-9+&@#/%?=~_|!:,.;]+[-A-Za-z0-9+&@#/%=~_|])")
                     .exactMatch(item.url);
        if (!match) {
            QFileInfo fileInfo(item.url);
            if (!fileInfo.exists()) 
                goto throw_item;
        }

        /* check for duplicates */
        iterator = m_playlist;
        while (iterator.hasNext()) {
            PlaylistItem next = iterator.next();
            if (next.url == item.url)
                goto throw_item;
        }

        /* add a playlist item to list widget */
        widget = new(std::nothrow) QListWidgetItem();
        if (!widget)
            QApplication::exit(KENOMEM);
        info = item.url;
        widget->setText(match ? item.url : info.fileName());
        widget->setToolTip(item.url);
        widget->setBackgroundColor(QColor(51, 51, 51));
        widget->setIcon(m_iconVideo);
        item.widgetItem = widget;
        m_playlistWidget->addItem(widget);
        if (item.url == m_nextItem.url) {
            m_nextItem.widgetItem = widget;
            widget->setSelected(true);
        }

        /* add new item to playlist */
        m_playlist.append(item);
throw_item:
        continue;
    }
}

void KAVPlayer::savePlaylist()
{ 
    /* make playlist directory and playlist file */
    QString path = m_appDirPath + "/playlist/";
    QString fileName = path + "playlist.pl";
    QFileInfo file(fileName);
    QDir dir(path);
    if (!dir.exists(path) && !dir.mkpath(path))
        return;
    if (!file.isFile()) {
        FILE *fp = fopen(fileName.toLocal8Bit(), "w+");
        if (!fp)
            return;
        fclose(fp);
        fp = NULL;
    }

    /* save playlist and play history */
    if (m_autoCleanList) {
        m_playlist.clear();
        m_playlistWidget->clear();
    }
    IniFile saver;
    saver.setValue("PLAYLIST_VERSION", "VER_ID", VERSION);
    if (m_nextItem.widgetItem)
        saver.setValue("PLAY_HISTORY", "URL", m_nextItem.url.toStdString());
    int len = m_playlist.size();
    saver.setValue("PLAY_LIST", "LIST_LEN", std::to_string(len));
    QMutableLinkedListIterator<PlaylistItem> iterator(m_playlist);
    for (int i = 0; i < len; i++) {
        PlaylistItem item = iterator.next();
        saver.setValue("PLAY_LIST", "URL" + std::to_string(i), item.url.toStdString());
    }
    saver.saveas(fileName.toLocal8Bit().toStdString());
}

void KAVPlayer::loadIcons ()
{
    m_iconVideo = QIcon(m_appDirPath + "/icon/video.ico");
    m_iconList = QIcon(m_appDirPath + "/icon/list.ico");
    m_iconListCycle = QIcon(m_appDirPath + "/icon/list_cycle.ico");
    m_iconSingleCycle = QIcon(m_appDirPath + "/icon/single_cycle.ico");
    m_iconRadom= QIcon(m_appDirPath + "/icon/random.ico");
    m_iconSingle = QIcon(m_appDirPath + "/icon/single.ico");
    m_iconPause = QIcon(m_appDirPath + "/icon/pause.ico");
    m_iconPlay = QIcon(m_appDirPath + "/icon/play.ico");
    m_iconMaxVol = QIcon(m_appDirPath + "/icon/max_vol.ico");
    m_iconMute = QIcon(m_appDirPath + "/icon/mute.ico");
    m_iconYes = QIcon(m_appDirPath + "/icon/yes.ico");
    m_iconNo = QIcon(m_appDirPath + "/icon/no.ico");
    m_iconFile = QIcon(m_appDirPath + "/icon/file.ico");
    m_iconUrl = QIcon(m_appDirPath + "/icon/url.ico");
}

int KAVPlayer::initWindow ()
{
    int ret;

    /* init window */
    setGeometry(m_windowRect.toQRect());
    setMinimumSize(m_showList ? MIN_WINDOW_W : MIN_WINDOW_W_NOLIST,
                   MIN_WINDOW_H);
//    setStyleSheet("background-color:rgb(0, 0, 0)");

    /* init player pane */
    m_playerPane = new(std::nothrow) QWidget();
    if (!m_playerPane)
        QApplication::exit(KENOMEM);
    m_playerPane->setGeometry(m_playerPaneRect.toQRect());
    m_playerPane->setMinimumWidth(MIN_WINDOW_W_NOLIST);
    m_playerPane->setStyleSheet("background-color:rgb(0, 0, 0)");

    /* init list pane */
    m_listPane = new(std::nothrow) QWidget();
    if (!m_listPane)
        QApplication::exit(KENOMEM);
    m_listPane->setGeometry(m_listPaneRect.toQRect());
    m_listPane->setStyleSheet("background-color:rgb(51, 51, 51)");
    if (!m_showList)
        m_listPane->hide();

    /* init splitter */   
    m_splitter = new(std::nothrow) QSplitter(Qt::Horizontal, this);
    if (!m_splitter)
        QApplication::exit(KENOMEM);
    m_splitter->setHandleWidth(FRAME_W);
    m_splitter->setStyleSheet("background-color:rgb(0, 0, 0)");
    m_splitter->addWidget(m_playerPane);
    m_splitter->addWidget(m_listPane);
    m_splitter->setStretchFactor(0, m_playerPaneRect.w);
    QObject::connect(m_splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(changeListWidth(int)));

    /* init playlist widget */
    m_playlistWidget = new(std::nothrow) QListWidget(m_listPane);
    if (!m_playlistWidget)
        QApplication::exit(KENOMEM);
    m_playlistWidget->setStyleSheet("QListWidget{background-color:rgb(40, 40, 40);color:white;border:0px;font-size:14px}"
                                    "QListWidget::item:selected{background:gray}");
    m_playlistWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_playlistWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QObject::connect(m_playlistWidget, SIGNAL(itemDoubleClicked(QListWidgetItem *)), 
                     this, SLOT(playListItem(QListWidgetItem *)));
    QObject::connect(m_playlistWidget, SIGNAL(itemClicked(QListWidgetItem *)), 
                     this, SLOT(selListItem(QListWidgetItem *)));

    /* init file open button */
    m_open = new(std::nothrow) QPushButton(m_listPane);
    if (!m_open)
        QApplication::exit(KENOMEM);
    m_open->setIcon(QIcon(m_appDirPath + "/icon/open.ico"));
    m_open->setStyleSheet(BTN_STYLE_SHEET1);
    m_openMenu = new(std::nothrow) QMenu(this);
    m_openMenu->setStyleSheet("background-color:rgb(40, 40, 40);color:white");
    m_openMenu->addAction(m_iconFile, "Open File", this, SLOT(openFile()));
    m_openMenu->addSeparator();
    m_openMenu->addAction(m_iconUrl, "Open URL", this, SLOT(openUrl()));
    m_open->setMenu(m_openMenu);

    /* init item delete button */
    m_delete = new(std::nothrow) QPushButton(m_listPane);
    if (!m_delete)
        QApplication::exit(KENOMEM);
    m_delete->setIcon(QIcon(m_appDirPath + "/icon/delete.ico"));
    m_delete->setStyleSheet(BTN_STYLE_SHEET1);
    QObject::connect(m_delete, SIGNAL(clicked()), this, SLOT(deleteItem()));
    
    /* init play mode switch button */
    m_playModeSwitch = new(std::nothrow) QPushButton(m_listPane);
    if (!m_playModeSwitch)
        QApplication::exit(KENOMEM);
    switch (m_playMode) {
    case PLAY_MODE_LIST_SEQUENCE:
        m_playModeSwitch->setIcon(m_iconList);
        break;
    case PLAY_MODE_LIST_CYCLE:
        m_playModeSwitch->setIcon(m_iconListCycle);
        break;
    case PLAY_MODE_SIGNAL_CYCLE:
        m_playModeSwitch->setIcon(m_iconSingleCycle);
        break;
    case PLAY_MODE_RANDOM:
        m_playModeSwitch->setIcon(m_iconRadom);
        break;
    case PLAY_MODE_SINGLE:
        m_playModeSwitch->setIcon(m_iconSingle);
        break;
    }
    m_playModeSwitch ->setStyleSheet(BTN_STYLE_SHEET1);
    QObject::connect(m_playModeSwitch, SIGNAL(clicked()), this, SLOT(nextPlayMode()));

    /* init setting button */
    m_setting = new(std::nothrow) QPushButton(m_listPane);
    if (!m_setting)
        QApplication::exit(KENOMEM);
    m_setting->setIcon(QIcon(m_appDirPath + "/icon/setting.ico"));
    m_setting->setStyleSheet(BTN_STYLE_SHEET1);
    m_settingMenu = new(std::nothrow) QMenu(this);
    m_settingMenu->setStyleSheet("background-color:rgb(40, 40, 40);color:white");
    // open
    m_settingMenu->addAction(m_iconFile, "Open file", this, SLOT(openFile()));
    m_settingMenu->addAction(m_iconUrl, "Open Url", this, SLOT(openUrl()));
    m_settingMenu->addSeparator();
    // window
    m_settingMenu->addAction(QIcon(!m_showList ? m_iconNo : m_iconYes), 
                             "Show list", this, SLOT(switchList()))->setObjectName("0-0");
    m_settingMenu->addAction(QIcon(m_topMost ? m_iconNo : m_iconYes), 
                             "Top most", this, SLOT(switchTopMost()))->setObjectName("0-1");
    m_settingMenu->addAction(QIcon(!m_autoFullscreen ? m_iconNo : m_iconYes), 
                             "Auto Fullscreen when playing", this, SLOT(switchAutoFullscr()))->setObjectName("0-2");
    m_settingMenu->addAction(QIcon(!m_savePos ? m_iconNo : m_iconYes), 
                             "Saving current position of window when closed", this, SLOT(switchSavePos()))->setObjectName("0-3");
    m_settingMenu->addAction(QIcon(!m_saveSize ? m_iconNo : m_iconYes), 
                             "Saving current size of window when closed", this, SLOT(switchSaveSize()))->setObjectName("0-4");
    m_settingMenu->addSeparator();
    // play list
    m_settingMenu->addAction(QIcon(!m_autoCleanList ? m_iconNo : m_iconYes), 
                             "Cleaning the list when closed", this, SLOT(switchAutoCleanList()))->setObjectName("1-0");
    m_settingMenu->addSeparator();
    // player
    QMenu *seekingModeMenu = m_settingMenu->addMenu(QIcon(m_appDirPath + "/icon/pos.ico"), "Seeking mode");
    seekingModeMenu->setObjectName("2-0");
    seekingModeMenu->addAction(QIcon(m_fastSeek ? m_iconNo : m_iconYes), 
                               "Exact seeking mode", this, SLOT(switchToExactSeekMode()))->setObjectName("2-0-0");
    seekingModeMenu->addAction(QIcon(!m_fastSeek ? m_iconNo : m_iconYes), 
                               "Fast seeking mode", this, SLOT(switchToFastSeekMode()))->setObjectName("2-0-1");
    m_settingMenu->addAction(QIcon(m_appDirPath + "/icon/step.ico"), 
                               "Step forward a frame", this, SLOT(step()))->setObjectName("2-1");
    m_settingMenu->addSeparator();
    // device
    m_settingMenu->addAction(QIcon(!m_hwAcce ? m_iconNo : m_iconYes), 
                               "Hardware Acceleration", this, SLOT(switchHWAcce()))->setObjectName("3-0");
    QMenu *audioDeviceMenu = m_settingMenu->addMenu(QIcon(m_appDirPath + "/icon/audio_device.ico"), "Audio device");
    audioDeviceMenu->setObjectName("3-1");
    // file info
    m_settingMenu->addSeparator();
    m_settingMenu->addAction(QIcon(m_appDirPath + "/icon/info.ico"), "File information", this, SLOT(showMediaFileInfo()));
    m_setting->setMenu(m_settingMenu);

    /* init list clear button */
    m_clear = new(std::nothrow) QPushButton(m_listPane);
    if (!m_clear)
        QApplication::exit(KENOMEM);
    m_clear->setStyleSheet("background-color:rgb(40, 40, 40);color:white"
                            "0px;border-radius:5px;font-size:20px");
    m_clear->setIcon(QIcon(m_appDirPath + "/icon/clear.ico"));
    m_clear->setStyleSheet(BTN_STYLE_SHEET1);
    QObject::connect(m_clear, SIGNAL(clicked()), this, SLOT(clearList()));

    /* init progress bar rect */
    m_progressPane = new(std::nothrow) QWidget(m_playerPane);
    if (!m_progressPane)
        QApplication::exit(KENOMEM);
    m_progressPane->setGeometry(m_progressPaneRect.toQRect());
    m_progressPane->setStyleSheet("background-color:rgb(40, 40, 40)");

    /* init control bar rect */
    m_controlBar = new(std::nothrow) QWidget(m_playerPane);
    if (!m_controlBar)
        QApplication::exit(KENOMEM);
    m_controlBar->setGeometry(m_controlBarRect.toQRect());
    m_controlBar->setStyleSheet("background-color:rgb(0, 0, 0)");

    /* init stop button */
    m_stop = new(std::nothrow) QPushButton(m_controlBar);
    if (!m_stop)
        QApplication::exit(KENOMEM);
    m_stop->setIcon(QIcon(m_appDirPath + "/icon/stop.ico"));
    m_stop->setStyleSheet(BTN_STYLE_SHEET);
    QObject::connect(m_stop, SIGNAL(clicked()), this, SLOT(stop()));

    /* init privious button */
    m_priv = new(std::nothrow) QPushButton(m_controlBar);
    if (!m_priv)
        QApplication::exit(KENOMEM);
    m_priv->setIcon(QIcon(m_appDirPath + "/icon/priv.ico"));
    m_priv->setStyleSheet(BTN_STYLE_SHEET);
    QObject::connect(m_priv, SIGNAL(clicked()), this, SLOT(priv()));

    /* init pause button */
    m_pause = new(std::nothrow) QPushButton(m_controlBar);
    if (!m_pause)
        QApplication::exit(KENOMEM);
    m_pause->setIcon(m_iconPlay);
    m_pause->setStyleSheet(BTN_STYLE_SHEET);
    QObject::connect(m_pause, SIGNAL(clicked()), this, SLOT(pause()));

    /* init next button */
    m_next = new(std::nothrow) QPushButton(m_controlBar);
    if (!m_next)
        QApplication::exit(KENOMEM);
    m_next->setIcon(QIcon(m_appDirPath + "/icon/next.ico"));
    m_next->setStyleSheet(BTN_STYLE_SHEET);
    QObject::connect(m_next, SIGNAL(clicked()), this, SLOT(next()));

    /* init list switch */
    m_listSwitch = new(std::nothrow) QPushButton(m_controlBar);
    if (!m_listSwitch)
        QApplication::exit(KENOMEM);
    m_listSwitch->setIcon(QIcon(m_appDirPath + "/icon/playlist.ico"));
    m_listSwitch->setStyleSheet(BTN_STYLE_SHEET);
    QObject::connect(m_listSwitch, SIGNAL(clicked()), this, SLOT(switchList()));

    /* init information label */
    m_infoLabel = new(std::nothrow) QLabel(m_controlBar);
    if (!m_infoLabel)
        QApplication::exit(KENOMEM);
    m_infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_infoLabel->setStyleSheet("background-color:rgb(40, 40, 40);color:white;border:0p");
    m_infoLabel->setText(" 00:00:00.000 / 00:00:00.000");

    /* init progress slider */
    m_progressSlider = new(std::nothrow) ClickSlider(Qt::Horizontal, m_progressPane);
    m_progressSlider->setRange(0, m_progressSlider->width());
    m_progressSlider->setSliderPosition(0);
    m_progressSlider->setStyleSheet("background-color:rgb(40, 40, 40)");
//    QObject::connect(m_progressSlider, SIGNAL(sliderPressed()), this, SLOT(stopUpdateProgressPos()));
//    QObject::connect(m_progressSlider, SIGNAL(sliderReleased()), this, SLOT(seek()));
    QObject::connect(m_progressSlider, SIGNAL(clicked()), this, SLOT(seek()));
    m_progressSlider->setDisabled(true);

    /* init mute button */
    m_mute = new(std::nothrow) QPushButton(m_progressPane);
    if (!m_mute)
        QApplication::exit(KENOMEM);
    m_vol ? m_mute->setIcon(m_iconMaxVol) : m_mute->setIcon(m_iconMute);
    m_mute->setStyleSheet(BTN_STYLE_SHEET);
    QObject::connect(m_mute, SIGNAL(clicked()), this, SLOT(switchMute()));

    /* init volume slider */
    m_volumeSlider = new(std::nothrow) ClickSlider(Qt::Horizontal, m_progressPane);
    m_volumeSlider->setRange(0, 64);
    m_volumeSlider->setSliderPosition(m_vol);
    m_progressSlider->setStyleSheet("background-color:rgb(40, 40, 40)");
    QObject::connect(m_volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(setVolume(int)));

    /* init video widget */
    m_videoWidget = new(std::nothrow) AVPlayerWidget(m_playerPane);
    if (!m_videoWidget)
        QApplication::exit(KENOMEM);
    m_videoWidget->setGeometry(m_videoRect.toQRect());
    QObject::connect(m_videoWidget, SIGNAL(player_stopped(int)), this, SLOT(errProc(int)));
    QObject::connect(m_videoWidget, SIGNAL(pos_changed(double)), this, SLOT(updatePorgressPos(double)), Qt::QueuedConnection); 
// progress slider and info label can't update, why?
//////////////////////////////////////

    /* init msg label */
    m_msgLabel = new(std::nothrow) QLabel(m_playerPane);
    if (m_msgLabel)
        QApplication::exit(KENOMEM);
    QSize scr_size = QApplication::desktop()->size();
    m_msgLabel->setStyleSheet("background-color:transparent;color:red;font-size:18px");
    m_msgLabel->hide();

    /* set focus */
    setFocus();

    return 0;
}

int KAVPlayer::run ()
{
    int ret;

    /* get application path */
    m_appDirPath = QCoreApplication::applicationDirPath();

    /* init logger */
    QString path = m_appDirPath + "/log";
    QDir dir(path);
    if (!dir.exists(path) && !dir.mkpath(path))
        ;
    ret = logger.init((m_appDirPath + "/log/test.log").toLocal8Bit());
    if (ret < 0) {
        QMessageBox::information(this, "KAVPlayer", getErrString(KEGUI_LOG_INIT_FAIL), QMessageBox::Ok);
        return ret;
    }
    logger.info("==================================\n");
    logger.info("%s\n", QDateTime::currentDateTime().toString().toStdString().c_str());
    logger.info("Application Path: %s\n", m_appDirPath.toStdString().c_str());

    /* set cursor */
    this->setCursor(Qt::WaitCursor);

    /* load setting */
    loadSetting();

    /* load icons */
    loadIcons();

    /* init window */
    ret = initWindow();
    if (ret < 0) {
        QMessageBox::information(this, "KAVPlayer", getErrString(KEGUI_WINDOW_INIT_FAIL), QMessageBox::Ok);
        logger.fatal("%s.\n", getErrString(KEGUI_WINDOW_INIT_FAIL));
        return ret;
    }

    /* load playlist */
    loadPlaylist();
    
    /* show window */
    show();

    m_topMost = !m_topMost;
    switchTopMost();

    /* init player widget */
    ret = m_videoWidget->init(false/*m_hwAcce*/); //////////////////////
    if (ret < 0) {
        QMessageBox::information(this, "KAVPlayer", getErrString(KEGUI_VIDEO_WIDGET_INIT_FAIL), QMessageBox::Ok);
        logger.fatal("%s.\n", getErrString(KEGUI_VIDEO_WIDGET_INIT_FAIL));
        return ret;
    }
    
    /* set cursor */
    this->setCursor(Qt::ArrowCursor);

    return 0;
}

KAVPlayer::KAVPlayer ()
    : QMainWindow(nullptr)
{
    /* init menbers */
    m_leftButtonDown = false;
    m_duration = 0.0;
    m_currentPos = 0.0;
    m_stopUpdateProgressPos = false;
    m_playerWidget = NULL;
    m_playerPane = NULL;
    m_listPane = NULL;
    m_progressPane = NULL;
    m_controlBar = NULL;
    m_videoWidget = NULL;
    m_stop = NULL;
    m_priv = NULL;
    m_pause = NULL;
    m_next = NULL;
    m_listSwitch = NULL;
    m_mute = NULL;
    m_infoLabel = NULL;
    m_msgLabel = NULL;
    m_progressSlider = NULL;
    m_volumeSlider = NULL;
    m_playlistWidget = NULL;
    m_delete = NULL;
    m_open = NULL;
    m_openMenu = NULL;
    m_playModeSwitch = NULL;
    m_clear = NULL;
    m_setting = NULL;
}

KAVPlayer::~KAVPlayer ()
{
    /* free playlist */
    QMutableLinkedListIterator<PlaylistItem> iterator(m_playlist);
    while (iterator.hasNext()) {
        PlaylistItem &item = iterator.next();
        delete item.widgetItem;
        item.widgetItem = NULL;
    }
    m_playlist.clear();
    if (m_playlistWidget)
        m_playlistWidget->clear();

    /* free members */
    delete m_videoWidget;
    delete m_stop;
    delete m_priv;
    delete m_pause;
    delete m_next;
    delete m_listSwitch;
    delete m_mute;
    delete m_infoLabel;
    delete m_msgLabel;
    delete m_progressSlider;
    delete m_volumeSlider;
    delete m_playlistWidget;
    delete m_delete;
    delete m_open;
    delete m_openMenu;
    delete m_playModeSwitch;
    delete m_clear;
    delete m_setting;
    delete m_progressPane;
    delete m_controlBar;
    delete m_playerPane;
    delete m_listPane;
    delete m_playerWidget;
}
