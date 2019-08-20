#ifndef _KAVPLAYER_ /* _KAVPLAYER_ */
#define _KAVPLAYER_ 

#include <QtWidgets/QMainWindow>
#include <QEvent>
#include <QDialog>
#include <QPushButton>
#include <QSplitter>
#include <QLabel>
#include <QMenu>
#include <QIcon>
#include <QTextEdit>
#include <QTimer>
#include <QSlider>
#include <QLinkedList>
#include <QListWidgetItem>
#include <QListWidget>
#include <iterator>
#include <QWidget>
#include "ClickSlider.h"
#include "AVPlayerWidget.h"

/* application version */
#define VERSION                 "1.0"

/* widget size */
#define FRAME_W                 3
#define DIV_LINE_W              1
#define PROGRESS_BAR_H          20
#define VOLUME_BAR_W            64
#define BTN_W                   40
#define BTN1_DIV_W              5
#define BTN1_W                  BTN_W * 2 / 3
#define MIN_INFO_LABEL_W        170
#define MIN_LIST_W              200
#define MIN_WINDOW_W_NOLIST     (MIN_INFO_LABEL_W + 5 * BTN_W + 5 * DIV_LINE_W)
#define MIN_WINDOW_W            (MIN_WINDOW_W_NOLIST + MIN_LIST_W + FRAME_W)
#define MIN_WINDOW_H            (PROGRESS_BAR_H + BTN_W + 1 * DIV_LINE_W)
#define DEF_WINDOW_W            800
#define DEF_WINDOW_H            500
#define PLAYER_PANE_X           FRAME_W
#define MSG_LABEL_H             40
/* max volume */
#define MAX_VOL                 64

/* button style sheet */
;
#define BTN_STYLE_SHEET  "QPushButton{background-color:rgb(40, 40, 40);border:0px}"\
                         "QPushButton:hover{background-color:rgb(128, 128, 128);border:0px}"\
                         "QPushButton:pressed{background-color:rgb(24, 24, 24);border:0px}"
#define BTN_STYLE_SHEET1 "QPushButton{background-color:rgb(40, 40, 40);border:0px;border-radius:5px}"\
                         "QPushButton:hover{background-color:rgb(128, 128, 128);border:0px;border-radius:5px}"\
                         "QPushButton:pressed{background-color:rgb(24, 24, 24);border:0px;border-radius:5px}"

enum PlayMode {
    PLAY_MODE_LIST_SEQUENCE = 0,
    PLAY_MODE_LIST_CYCLE = 1,
    PLAY_MODE_SIGNAL_CYCLE = 2,
    PLAY_MODE_RANDOM = 3,
    PLAY_MODE_SINGLE = 4
};

struct Rect {
    int x;
    int y;
    int w;
    int h;
    QRect toQRect ();
    void  setFromQRect (QRect rect);
};

enum FileType {
    FILE_TYPE_UNKNOWN = -1,
    FILE_TYPE_VIDEO = 0,
    FILE_TYPE_AUDIO = 1,
    FILE_TYPE_PICTURE = 2
};

struct PlaylistItem {
    QString          url;
    QListWidgetItem *widgetItem;
};

class KAVPlayer : public QMainWindow {
    Q_OBJECT

private:
    bool                      m_leftButtonDown;
    QPoint                    m_startPoint;
    QPoint                    m_windowPoint;
    QString                   m_lastOpenedPath;
    bool                      m_stopUpdateProgressPos;
    QString                   m_strDuration;
    PlaylistItem              m_nextItem;
    QTimer                    m_msgLabelTimer;

private:
    /* widgets */
    AVPlayerWidget *          m_playerWidget;
    QSplitter *               m_splitter;
    QWidget *                 m_playerPane;
    QWidget *                 m_listPane;
    QWidget *                 m_progressPane;
    QWidget *                 m_controlBar;
    AVPlayerWidget*           m_videoWidget;
    QPushButton *             m_stop;
    QPushButton *             m_priv;
    QPushButton *             m_pause;
    QPushButton *             m_next;
    QPushButton *             m_listSwitch;
    QPushButton *             m_mute;
    QLabel *                  m_infoLabel;
    QLabel *                  m_msgLabel;
    ClickSlider *             m_progressSlider;
    ClickSlider *             m_volumeSlider;
    QListWidget *             m_playlistWidget;
    QPushButton *             m_delete;
    QPushButton *             m_open;
    QMenu *                   m_openMenu;
    QMenu *                   m_settingMenu;
    QPushButton *             m_playModeSwitch;
    QPushButton *             m_setting;
    QPushButton *             m_clear;

private:
    /* setting */
    Rect                      m_windowRect;
    Rect                      m_playerPaneRect;
    Rect                      m_listPaneRect;
    Rect                      m_progressPaneRect;
    Rect                      m_controlBarRect;
    Rect                      m_videoRect;
    int                       m_vol;
    int                       m_oldVol;
    bool                      m_showList;
    double                    m_duration;
    double                    m_currentPos;
    int                       m_playMode;
    bool                      m_topMost;
    bool                      m_fastSeek;
    bool                      m_autoCleanList;
    bool                      m_hwAcce;
    int                       m_audioDevice;
    bool                      m_autoFullscreen;
    bool                      m_savePos;
    bool                      m_saveSize;

private:
    /* icons */
    QIcon                     m_iconVideo;
    QIcon                     m_iconList;
    QIcon                     m_iconListCycle;
    QIcon                     m_iconSingleCycle;
    QIcon                     m_iconRadom;
    QIcon                     m_iconSingle;
    QIcon                     m_iconPause;
    QIcon                     m_iconPlay;
    QIcon                     m_iconMaxVol;
    QIcon                     m_iconMute;
    QIcon                     m_iconYes;
    QIcon                     m_iconNo;
    QIcon                     m_iconFile;
    QIcon                     m_iconUrl;

private:
    QString                   m_appDirPath;

private:
    /* playlist*/
    QLinkedList<PlaylistItem> m_playlist;

private slots:
    void errProc               (int err_code);
    void stop                  ();
    void priv                  ();
    void pause                 ();
    void next                  ();
    void switchList            ();
    void setVolume             (int value);
    void switchMute            ();
    void seek                  ();
    void updatePorgressPos     (double pos);
    void stopUpdateProgressPos ();
    void selListItem           (QListWidgetItem *item);
    void playListItem          (QListWidgetItem *item);
    void openFile              ();
    void openUrl               ();
    void deleteItem            ();
    void clearList             ();
    void playNextListItem      ();
    void nextPlayMode          ();
    void changeListWidth       (int pos);
    void switchTopMost         ();
    void switchAutoFullscr     ();
    void switchSavePos         ();
    void switchSaveSize        ();
    void switchAutoCleanList   ();
    void switchToExactSeekMode ();
    void switchToFastSeekMode  ();
    void step                  ();
    void showMediaFileInfo     ();
    void switchHWAcce          ();

private:
    void resizeEvent           (QResizeEvent *e);
    void changeEvent           (QEvent * e);
    void mousePressEvent       (QMouseEvent *e);
    void mouseReleaseEvent     (QMouseEvent *e);
    void mouseMoveEvent        (QMouseEvent *e);
    void keyPressEvent         (QKeyEvent *e);
    void closeEvent            (QCloseEvent *e);

private:
    void selNextListItem       ();
    void loadSetting           ();
    void saveSetting           ();
    void loadPlaylist          ();
    void savePlaylist          ();
    void loadIcons             ();
    int  initWindow            ();

public:
    int  run                   ();

private:
    KAVPlayer                  (QWidget *parent);

public:
    KAVPlayer                  ();
    ~KAVPlayer                 ();;
};

#endif /* _KAVPLAYER_ */