//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id: preferences.cpp 5660 2012-05-22 14:17:39Z wschweer $
//
//  Copyright (C) 2002-2011 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "config.h"
#include "libmscore/xml.h"
#include "libmscore/score.h"
#include "musescore.h"
#include "preferences.h"
#include "prefsdialog.h"
#include "msynth/synti.h"
#include "seq.h"
#include "libmscore/note.h"
#include "playpanel.h"
#include "icons.h"
#include "shortcutcapturedialog.h"
#include "scoreview.h"
#include "libmscore/sym.h"
#include "palette.h"
#include "pa.h"
#include "pm.h"
#include "libmscore/page.h"
#include "file.h"
#include "libmscore/mscore.h"
#include "shortcut.h"
#include "plugins.h"

bool useALSA = false, useJACK = false, usePortaudio = false;

extern bool useFactorySettings;
extern bool externalStyle;
extern QString iconGroup;

static const char* appStyleFile;
static int exportAudioSampleRates[2] = { 44100, 48000 };

//---------------------------------------------------------
//   PeriodItem
//---------------------------------------------------------

struct PeriodItem {
       int time;
       const char* text;
       PeriodItem(const int t, const  char* txt) {
             time = t;
             text = txt;
             }
       };

static PeriodItem updatePeriods[] = {
      PeriodItem(24,      QT_TRANSLATE_NOOP("preferences","Every day")),
      PeriodItem(72,      QT_TRANSLATE_NOOP("preferences","Every 3 days")),
      PeriodItem(7*24,    QT_TRANSLATE_NOOP("preferences","Every week")),
      PeriodItem(2*7*24,  QT_TRANSLATE_NOOP("preferences","Every 2 weeks")),
      PeriodItem(30*24,   QT_TRANSLATE_NOOP("preferences","Every month")),
      PeriodItem(2*30*24, QT_TRANSLATE_NOOP("preferences","Every 2 months")),
      PeriodItem(-1,      QT_TRANSLATE_NOOP("preferences","Never")),
      };

//---------------------------------------------------------
//   appStyleSheet
//---------------------------------------------------------

QString appStyleSheet()
      {
      QString s;
      QFile f(appStyleFile);
      if (f.open(QIODevice::ReadOnly)) {
            s = f.readAll();
            f.close();
            }
      return s;
      }

//---------------------------------------------------------
//   Preferences
//---------------------------------------------------------

Preferences preferences;

Preferences::Preferences()
      {
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void Preferences::init()
      {
      // set fallback defaults:

      bgUseColor         = true;
      fgUseColor         = false;
      bgWallpaper        = QString();
      fgWallpaper        = ":/data/paper5.png";
      fgColor.setRgb(255, 255, 255);
      iconHeight         = 25;
      iconWidth          = 20;

      enableMidiInput    = true;
      playNotes          = true;
      lPort              = "";
      rPort              = "";

      showNavigator      = true;
      showPlayPanel      = false;
      showWebPanel       = true;
      showStatusBar      = true;
      playPanelPos       = QPoint(100, 300);

#if defined(Q_WS_MAC) || defined(__MINGW32__)
      useAlsaAudio       = false;
      useJackAudio       = false;
      usePortaudioAudio  = true;
      usePulseAudio      = false;
      useJackMidi        = false;
#else
      useAlsaAudio       = false;
      useJackAudio       = false;
      usePortaudioAudio  = false;
      usePulseAudio      = true;
      useJackMidi        = false;
#endif

      midiPorts          = 2;
      rememberLastMidiConnections = true;

      alsaDevice         = "default";
      alsaSampleRate     = 48000;
      alsaPeriodSize     = 1024;
      alsaFragments      = 3;
      portaudioDevice    = -1;
      portMidiInput      = "";

      antialiasedDrawing       = true;
      sessionStart             = SCORE_SESSION;
      startScore               = ":/data/Promenade_Example.mscz";
      defaultStyleFile         = "";
      showSplashScreen         = true;

      useMidiRemote      = false;
      for (int i = 0; i < MIDI_REMOTES; ++i)
            midiRemote[i].type = MIDI_REMOTE_TYPE_INACTIVE;

      midiExpandRepeats        = true;
      MScore::playRepeats      = true;
      MScore::panPlayback      = true;
      instrumentList1          = ":/data/instruments.xml";
      instrumentList2          = "";

      musicxmlImportLayout     = true;
      musicxmlImportBreaks     = true;
      musicxmlExportLayout     = true;
      musicxmlExportBreaks     = ALL_BREAKS;

      alternateNoteEntryMethod = false;
      proximity                = 6;
      autoSave                 = true;
      autoSaveTime             = 2;       // minutes
      pngResolution            = 300.0;
      pngTransparent           = true;
      language                 = "system";

      replaceCopyrightSymbol  = true;

      mag                     = 1.0;

      checkUpdateStartup      = 0;

      tuning                  = 440.0f;
      masterGain              = 0.2;
      chorusGain              = 0.5;
      reverbGain              = 0.5;
      reverbRoomSize          = 0.5;
      reverbDamp              = 0.5;
      reverbWidth             = 1.0;

      followSong              = true;
      importCharset           = "GBK";
      importStyleFile         = "";
      shortestNote            = MScore::division/4;

      useOsc                  = false;
      oscPort                 = 5282;
      appStyleFile            = ":/data/appstyle-dark.css";
      singlePalette           = false;

      styleName               = "dark";   // ??
      globalStyle             = STYLE_DARK;

      QString workingDirectory = QString("%1/%2").arg(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).arg(QCoreApplication::applicationName());
      myScoresPath            = QDir(QString("%1/%2").arg(workingDirectory).arg(QCoreApplication::translate("scores_directory", "Scores"))).absolutePath();
      myStylesPath            = QDir(QString("%1/%2").arg(workingDirectory).arg(QCoreApplication::translate("styles_directory", "Styles"))).absolutePath();
      myImagesPath            = QDir(QString("%1/%2").arg(workingDirectory).arg(QCoreApplication::translate("images_directory", "Images"))).absolutePath();
      myTemplatesPath         = QDir(QString("%1/%2").arg(workingDirectory).arg(QCoreApplication::translate("templates_directory", "Templates"))).absolutePath();
      myPluginsPath           = QDir(QString("%1/%2").arg(workingDirectory).arg(QCoreApplication::translate("plugins_directory", "Plugins"))).absolutePath();
      mySoundFontsPath        = QDir(QString("%1/%2").arg(workingDirectory).arg(QCoreApplication::translate("soundfonts_directory", "Soundfonts"))).absolutePath();

      nudgeStep10             = 1.0;      // Ctrl + cursor key (default 1.0)
      nudgeStep50             = 5.0;      // Alt  + cursor key (default 5.0)

      MScore::setHRaster(2);        // _spatium / value
      MScore::setVRaster(2);
#if defined(Q_WS_MAC) || defined(__MINGW32__)
      // use system native file dialogs
      // Qt file dialog is very slow on Windows and Mac
      nativeDialogs           = true;
#else
      nativeDialogs           = false;    // don't use system native file dialogs
#endif

      exportAudioSampleRate   = exportAudioSampleRates[0];

      workspace                 = "default";

      firstStartWeb = true;
      };

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Preferences::write()
      {
      dirty = false;
      QSettings s;

      s.setValue("bgUseColor",         bgUseColor);
      s.setValue("fgUseColor",         fgUseColor);
      s.setValue("bgWallpaper",        bgWallpaper);
      s.setValue("fgWallpaper",        fgWallpaper);
      s.setValue("fgColor",            fgColor);
      s.setValue("bgColor",            MScore::bgColor);
      s.setValue("iconHeight",         iconHeight);
      s.setValue("iconWidth",          iconWidth);

      s.setValue("selectColor1",       MScore::selectColor[0]);
      s.setValue("selectColor2",       MScore::selectColor[1]);
      s.setValue("selectColor3",       MScore::selectColor[2]);
      s.setValue("selectColor4",       MScore::selectColor[3]);
      s.setValue("dropColor",          MScore::dropColor);
      s.setValue("defaultColor",       MScore::defaultColor);
      s.setValue("enableMidiInput",    enableMidiInput);
      s.setValue("playNotes",          playNotes);

      s.setValue("soundFont",          MScore::soundFont);
      s.setValue("lPort",              lPort);
      s.setValue("rPort",              rPort);
      s.setValue("showNavigator",      showNavigator);
      s.setValue("showPlayPanel",      showPlayPanel);
      s.setValue("showWebPanel",       showWebPanel);
      s.setValue("showStatusBar",      showStatusBar);

      s.setValue("useAlsaAudio",       useAlsaAudio);
      s.setValue("useJackAudio",       useJackAudio);
      s.setValue("useJackMidi",        useJackMidi);
      s.setValue("usePortaudioAudio",  usePortaudioAudio);
      s.setValue("usePulseAudio",      usePulseAudio);
      s.setValue("midiPorts",          midiPorts);
      s.setValue("rememberLastMidiConnections", rememberLastMidiConnections);

      s.setValue("alsaDevice",         alsaDevice);
      s.setValue("alsaSampleRate",     alsaSampleRate);
      s.setValue("alsaPeriodSize",     alsaPeriodSize);
      s.setValue("alsaFragments",      alsaFragments);
      s.setValue("portaudioDevice",    portaudioDevice);
      s.setValue("portMidiInput",   portMidiInput);

      s.setValue("layoutBreakColor",   MScore::layoutBreakColor);
      s.setValue("antialiasedDrawing", antialiasedDrawing);
      switch(sessionStart) {
            case EMPTY_SESSION:  s.setValue("sessionStart", "empty"); break;
            case LAST_SESSION:   s.setValue("sessionStart", "last"); break;
            case NEW_SESSION:    s.setValue("sessionStart", "new"); break;
            case SCORE_SESSION:  s.setValue("sessionStart", "score"); break;
            }
      s.setValue("startScore",         startScore);
      s.setValue("defaultStyle",       defaultStyleFile);
      s.setValue("partStyle",          MScore::partStyle);
      s.setValue("showSplashScreen",   showSplashScreen);

      s.setValue("midiExpandRepeats",  midiExpandRepeats);
      s.setValue("playRepeats",        MScore::playRepeats);
      s.setValue("panPlayback",        MScore::panPlayback);
      s.setValue("instrumentList",     instrumentList1);
      s.setValue("instrumentList2",    instrumentList2);

      s.setValue("musicxmlImportLayout",  musicxmlImportLayout);
      s.setValue("musicxmlImportBreaks",  musicxmlImportBreaks);
      s.setValue("musicxmlExportLayout",  musicxmlExportLayout);
      switch(musicxmlExportBreaks) {
            case ALL_BREAKS:     s.setValue("musicxmlExportBreaks", "all"); break;
            case MANUAL_BREAKS:  s.setValue("musicxmlExportBreaks", "manual"); break;
            case NO_BREAKS:      s.setValue("musicxmlExportBreaks", "no"); break;
            }

      s.setValue("alternateNoteEntry", alternateNoteEntryMethod);
      s.setValue("proximity",          proximity);
      s.setValue("autoSave",           autoSave);
      s.setValue("autoSaveTime",       autoSaveTime);
      s.setValue("pngResolution",      pngResolution);
      s.setValue("pngTransparent",     pngTransparent);
      s.setValue("language",           language);

      s.setValue("replaceFractions", MScore::replaceFractions);
      s.setValue("replaceCopyrightSymbol", replaceCopyrightSymbol);
      s.setValue("paperWidth",  MScore::defaultStyle()->pageFormat()->width());
      s.setValue("paperHeight", MScore::defaultStyle()->pageFormat()->height());

      s.setValue("twosided",    MScore::defaultStyle()->pageFormat()->twosided());
      s.setValue("spatium",     MScore::defaultStyle()->spatium() / MScore::DPI);

      s.setValue("mag", mag);
      s.setValue("tuning", tuning);
      s.setValue("masterGain", masterGain);
      s.setValue("chorusGain", chorusGain);
      s.setValue("reverbGain", reverbGain);
      s.setValue("reverbRoomSize", reverbRoomSize);
      s.setValue("reverbDamp", reverbDamp);
      s.setValue("reverbWidth", reverbWidth);

      s.setValue("defaultPlayDuration", MScore::defaultPlayDuration);
      s.setValue("importStyleFile", importStyleFile);
      s.setValue("shortestNote", shortestNote);
      s.setValue("importCharset", importCharset);
      s.setValue("warnPitchRange", MScore::warnPitchRange);
      s.setValue("followSong", followSong);

      s.setValue("useOsc", useOsc);
      s.setValue("oscPort", oscPort);
      s.setValue("style", styleName);
      s.setValue("singlePalette", singlePalette);

      s.setValue("myScoresPath", myScoresPath);
      s.setValue("myStylesPath", myStylesPath);
      s.setValue("myImagesPath", myImagesPath);
      s.setValue("myTemplatesPath", myTemplatesPath);
      s.setValue("myPluginsPath", myPluginsPath);
      s.setValue("mySoundFontsPath", mySoundFontsPath);

      s.setValue("hraster", MScore::hRaster());
      s.setValue("vraster", MScore::vRaster());
      s.setValue("nativeDialogs", nativeDialogs);
      s.setValue("exportAudioSampleRate", exportAudioSampleRate);

      s.setValue("workspace", workspace);

      s.setValue("firstStartWeb", firstStartWeb);

      //update
      s.setValue("checkUpdateStartup", checkUpdateStartup);

      s.setValue("useMidiRemote", useMidiRemote);
      for (int i = 0; i < MIDI_REMOTES; ++i) {
            if (midiRemote[i].type != MIDI_REMOTE_TYPE_INACTIVE) {
                  QChar t;
                  if (midiRemote[i].type == MIDI_REMOTE_TYPE_NOTEON)
                        t = QChar('P');
                  else
                        t = QChar('C');
                  s.setValue(QString("remote%1").arg(i),
                     QString("%1%2").arg(t).arg(midiRemote[i].data));
                  }
            }

      s.beginGroup("PlayPanel");
      s.setValue("pos", playPanelPos);
      s.endGroup();

      writePluginList();
      if (Shortcut::dirty)
            Shortcut::save();
      Shortcut::dirty = false;
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Preferences::read()
      {
      QSettings s;

      bgUseColor              = s.value("bgUseColor", bgUseColor).toBool();
      fgUseColor              = s.value("fgUseColor", fgUseColor).toBool();
      bgWallpaper             = s.value("bgWallpaper", bgWallpaper).toString();
      fgWallpaper             = s.value("fgWallpaper", fgWallpaper).toString();
      fgColor                 = s.value("fgColor", fgColor).value<QColor>();
      MScore::bgColor         = s.value("bgColor", MScore::bgColor).value<QColor>();
      iconHeight              = s.value("iconHeight", iconHeight).toInt();
      iconWidth               = s.value("iconWidth", iconWidth).toInt();

      MScore::selectColor[0]  = s.value("selectColor1", MScore::selectColor[0]).value<QColor>();
      MScore::selectColor[1]  = s.value("selectColor2", MScore::selectColor[1]).value<QColor>();
      MScore::selectColor[2]  = s.value("selectColor3", MScore::selectColor[2]).value<QColor>();
      MScore::selectColor[3]  = s.value("selectColor4", MScore::selectColor[3]).value<QColor>();

      MScore::defaultColor    = s.value("defaultColor", MScore::defaultColor).value<QColor>();
      MScore::dropColor       = s.value("dropColor",    MScore::dropColor).value<QColor>();

      enableMidiInput         = s.value("enableMidiInput", enableMidiInput).toBool();
      playNotes               = s.value("playNotes", playNotes).toBool();
      lPort                   = s.value("lPort", lPort).toString();
      rPort                   = s.value("rPort", rPort).toString();

      MScore::soundFont = s.value("soundFont", MScore::soundFont).toString();
      if (MScore::soundFont == ":/data/piano1.sf2") {
            // silently change to new default sound font
            MScore::soundFont = MScore::globalShare() + "sound/fluid.sf3";
            }
      showNavigator   = s.value("showNavigator", showNavigator).toBool();
      showStatusBar   = s.value("showStatusBar", showStatusBar).toBool();
      showPlayPanel   = s.value("showPlayPanel", showPlayPanel).toBool();
      showWebPanel    = s.value("showWebPanel", showWebPanel).toBool();

      useAlsaAudio       = s.value("useAlsaAudio", useAlsaAudio).toBool();
      useJackAudio       = s.value("useJackAudio", useJackAudio).toBool();
      useJackMidi        = s.value("useJackMidi",  useJackMidi).toBool();
      usePortaudioAudio  = s.value("usePortaudioAudio", usePortaudioAudio).toBool();
      usePulseAudio      = s.value("usePulseAudio", usePulseAudio).toBool();

      alsaDevice         = s.value("alsaDevice", alsaDevice).toString();
      alsaSampleRate     = s.value("alsaSampleRate", alsaSampleRate).toInt();
      alsaPeriodSize     = s.value("alsaPeriodSize", alsaPeriodSize).toInt();
      alsaFragments      = s.value("alsaFragments", alsaFragments).toInt();
      portaudioDevice    = s.value("portaudioDevice", portaudioDevice).toInt();
      portMidiInput      = s.value("portMidiInput", portMidiInput).toString();
      MScore::layoutBreakColor   = s.value("layoutBreakColor", MScore::layoutBreakColor).value<QColor>();
      antialiasedDrawing = s.value("antialiasedDrawing", antialiasedDrawing).toBool();

      defaultStyleFile         = s.value("defaultStyle", defaultStyleFile).toString();
      MScore::partStyle        = s.value("partStyle", MScore::partStyle).toString();

      showSplashScreen         = s.value("showSplashScreen", showSplashScreen).toBool();
      midiExpandRepeats        = s.value("midiExpandRepeats", midiExpandRepeats).toBool();
      MScore::playRepeats      = s.value("playRepeats", MScore::playRepeats).toBool();
      MScore::panPlayback      = s.value("panPlayback", MScore::panPlayback).toBool();
      alternateNoteEntryMethod = s.value("alternateNoteEntry", alternateNoteEntryMethod).toBool();
      midiPorts                = s.value("midiPorts", midiPorts).toInt();
      rememberLastMidiConnections = s.value("rememberLastMidiConnections", rememberLastMidiConnections).toBool();
      proximity                = s.value("proximity", proximity).toInt();
      autoSave                 = s.value("autoSave", autoSave).toBool();
      autoSaveTime             = s.value("autoSaveTime", autoSaveTime).toInt();
      pngResolution            = s.value("pngResolution", pngResolution).toDouble();
      pngTransparent           = s.value("pngTransparent", pngTransparent).toBool();
      language                 = s.value("language", language).toString();

      musicxmlImportLayout     = s.value("musicxmlImportLayout", musicxmlImportLayout).toBool();
      musicxmlImportBreaks     = s.value("musicxmlImportBreaks", musicxmlImportBreaks).toBool();
      musicxmlExportLayout     = s.value("musicxmlExportLayout", musicxmlExportLayout).toBool();
      QString br(s.value("musicxmlExportBreaks", "all").toString());
      if (br == "all")
            musicxmlExportBreaks = ALL_BREAKS;
      else if (br == "manual")
            musicxmlExportBreaks = MANUAL_BREAKS;
      else if (br == "no")
            musicxmlExportBreaks = NO_BREAKS;

      MScore::replaceFractions = s.value("replaceFractions", MScore::replaceFractions).toBool();
      replaceCopyrightSymbol = s.value("replaceCopyrightSymbol", replaceCopyrightSymbol).toBool();

      mag                    = s.value("mag", mag).toDouble();

      tuning                 = s.value("tuning", tuning).toDouble();
      masterGain             = s.value("masterGain",     masterGain).toDouble();
      chorusGain             = s.value("chorusGain",     chorusGain).toDouble();
      reverbGain             = s.value("reverbGain",     reverbGain).toDouble();
      reverbRoomSize         = s.value("reverbRoomSize", reverbRoomSize).toDouble();
      reverbDamp             = s.value("reverbDamp",     reverbDamp).toDouble();
      reverbWidth            = s.value("reverbWidth",    reverbWidth).toDouble();

      MScore::defaultPlayDuration = s.value("defaultPlayDuration", MScore::defaultPlayDuration).toInt();
      importStyleFile        = s.value("importStyleFile", importStyleFile).toString();
      shortestNote           = s.value("shortestNote", shortestNote).toInt();
      importCharset          = s.value("importCharset", importCharset).toString();
      MScore::warnPitchRange = s.value("warnPitchRange", MScore::warnPitchRange).toBool();
      followSong             = s.value("followSong", followSong).toBool();

      useOsc                 = s.value("useOsc", useOsc).toBool();
      oscPort                = s.value("oscPort", oscPort).toInt();
      styleName              = s.value("style", styleName).toString();
      if (styleName == "dark") {
            iconGroup = "icons-dark/";
            appStyleFile = ":/data/appstyle-dark.css";
            globalStyle  = STYLE_DARK;
            }
      else {
            iconGroup = "icons-dark/";
            appStyleFile = ":/data/appstyle.css";
            globalStyle  = STYLE_NATIVE;
            }

      singlePalette    = s.value("singlePalette",    singlePalette).toBool();

      myScoresPath     = s.value("myScoresPath",     myScoresPath).toString();
      myStylesPath     = s.value("myStylesPath",     myStylesPath).toString();
      myImagesPath     = s.value("myImagesPath",     myImagesPath).toString();
      myTemplatesPath  = s.value("myTemplatesPath",  myTemplatesPath).toString();
      myPluginsPath    = s.value("myPluginsPath",    myPluginsPath).toString();
      mySoundFontsPath = s.value("mySoundFontsPath", mySoundFontsPath).toString();

      //Create directories if they are missing
      QDir dir;
      dir.mkpath(myScoresPath);
      dir.mkpath(myStylesPath);
      dir.mkpath(myImagesPath);
      dir.mkpath(myTemplatesPath);
      dir.mkpath(myPluginsPath);
      dir.mkpath(mySoundFontsPath);

      MScore::setHRaster(s.value("hraster", MScore::hRaster()).toInt());
      MScore::setVRaster(s.value("vraster", MScore::vRaster()).toInt());

      nativeDialogs    = s.value("nativeDialogs", nativeDialogs).toBool();
      exportAudioSampleRate = s.value("exportAudioSampleRate", exportAudioSampleRate).toInt();

      workspace          = s.value("workspace", workspace).toString();

      firstStartWeb = s.value("firstStartWeb", true).toBool();

      checkUpdateStartup = s.value("checkUpdateStartup", checkUpdateStartup).toInt();
      if (checkUpdateStartup == 0)
            checkUpdateStartup = UpdateChecker::defaultPeriod();

      QString ss(s.value("sessionStart", "score").toString());
      if (ss == "last")
            sessionStart = LAST_SESSION;
      else if (ss == "new")
            sessionStart = NEW_SESSION;
      else if (ss == "score")
            sessionStart = SCORE_SESSION;
      else if (ss == "empty")
            sessionStart = EMPTY_SESSION;

      startScore     = s.value("startScore", startScore).toString();
      instrumentList1 = s.value("instrumentList",  instrumentList1).toString();
      instrumentList2 = s.value("instrumentList2", instrumentList2).toString();

      useMidiRemote  = s.value("useMidiRemote", useMidiRemote).toBool();
      for (int i = 0; i < MIDI_REMOTES; ++i) {
            QString data = s.value(QString("remote%1").arg(i)).toString();
            if (data.isEmpty())
                  midiRemote[i].type = MIDI_REMOTE_TYPE_INACTIVE;
            else {
                  midiRemote[i].data = data.mid(1).toInt();
                  if (data[0] == QChar('P')) {
                        midiRemote[i].type = MIDI_REMOTE_TYPE_NOTEON;
                        }
                  else if (data[0] == QChar('C')) {
                        midiRemote[i].type = MIDI_REMOTE_TYPE_CTRL;
                        }
                  }
            }

      s.beginGroup("PlayPanel");
      playPanelPos = s.value("pos", playPanelPos).toPoint();
      s.endGroup();

      readPluginList();
      }

//---------------------------------------------------------
//   preferences
//---------------------------------------------------------

void MuseScore::startPreferenceDialog()
      {
      if (!preferenceDialog) {
            preferenceDialog = new PreferenceDialog(this);
            connect(preferenceDialog, SIGNAL(preferencesChanged()),
               SLOT(preferencesChanged()));
            }
      preferenceDialog->setPreferences(preferences);
      preferenceDialog->show();
      }

//---------------------------------------------------------
//   PreferenceDialog
//---------------------------------------------------------

PreferenceDialog::PreferenceDialog(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
      startWithButton->setIcon(*icons[fileOpen_ICON]);
      instrumentList1Button->setIcon(*icons[fileOpen_ICON]);
      instrumentList2Button->setIcon(*icons[fileOpen_ICON]);
      defaultStyleButton->setIcon(*icons[fileOpen_ICON]);
      partStyleButton->setIcon(*icons[fileOpen_ICON]);
      myScoresButton->setIcon(*icons[fileOpen_ICON]);
      myStylesButton->setIcon(*icons[fileOpen_ICON]);
      myTemplatesButton->setIcon(*icons[fileOpen_ICON]);
      myPluginsButton->setIcon(*icons[fileOpen_ICON]);
      mySoundFontsButton->setIcon(*icons[fileOpen_ICON]);
      myImagesButton->setIcon(*icons[fileOpen_ICON]);

      bgWallpaperSelect->setIcon(*icons[fileOpen_ICON]);
      fgWallpaperSelect->setIcon(*icons[fileOpen_ICON]);
      sfOpenButton->setIcon(*icons[fileOpen_ICON]);
      styleFileButton->setIcon(*icons[fileOpen_ICON]);
      shortcutsChanged        = false;

#ifndef USE_JACK
      jackDriver->setVisible(false);
#endif
#ifndef USE_ALSA
      alsaDriver->setVisible(false);
#endif
#ifndef USE_PORTAUDIO
      portaudioDriver->setVisible(false);
#endif
#ifndef USE_PORTMIDI
      portmidiDriverInput->setVisible(false);
#endif
#ifndef USE_PULSEAUDIO
      pulseaudioDriver->setVisible(false);
#endif

      QButtonGroup* fgButtons = new QButtonGroup(this);
      fgButtons->setExclusive(true);
      fgButtons->addButton(fgColorButton);
      fgButtons->addButton(fgWallpaperButton);
      connect(fgColorButton, SIGNAL(toggled(bool)), SLOT(fgClicked(bool)));

      QButtonGroup* bgButtons = new QButtonGroup(this);
      bgButtons->setExclusive(true);
      bgButtons->addButton(bgColorButton);
      bgButtons->addButton(bgWallpaperButton);
      connect(bgColorButton, SIGNAL(toggled(bool)), SLOT(bgClicked(bool)));

      connect(buttonBox,          SIGNAL(clicked(QAbstractButton*)), SLOT(buttonBoxClicked(QAbstractButton*)));
      connect(fgWallpaperSelect,  SIGNAL(clicked()), SLOT(selectFgWallpaper()));
      connect(bgWallpaperSelect,  SIGNAL(clicked()), SLOT(selectBgWallpaper()));

      connect(myScoresButton, SIGNAL(clicked()), SLOT(selectScoresDirectory()));
      connect(myStylesButton, SIGNAL(clicked()), SLOT(selectStylesDirectory()));
      connect(myTemplatesButton, SIGNAL(clicked()), SLOT(selectTemplatesDirectory()));
      connect(myPluginsButton, SIGNAL(clicked()), SLOT(selectPluginsDirectory()));
      connect(mySoundFontsButton, SIGNAL(clicked()), SLOT(selectSoundFontsDirectory()));
      connect(myImagesButton, SIGNAL(clicked()), SLOT(selectImagesDirectory()));

      connect(defaultStyleButton,     SIGNAL(clicked()), SLOT(selectDefaultStyle()));
      connect(partStyleButton,        SIGNAL(clicked()), SLOT(selectPartStyle()));
      connect(instrumentList1Button,  SIGNAL(clicked()), SLOT(selectInstrumentList1()));
      connect(instrumentList2Button,  SIGNAL(clicked()), SLOT(selectInstrumentList2()));
      connect(startWithButton,        SIGNAL(clicked()), SLOT(selectStartWith()));

      connect(shortcutList,   SIGNAL(itemActivated(QTreeWidgetItem*, int)), SLOT(defineShortcutClicked()));
      connect(resetShortcut,  SIGNAL(clicked()), SLOT(resetShortcutClicked()));
      connect(clearShortcut,  SIGNAL(clicked()), SLOT(clearShortcutClicked()));
      connect(defineShortcut, SIGNAL(clicked()), SLOT(defineShortcutClicked()));
      connect(resetToDefault, SIGNAL(clicked()), SLOT(resetAllValues()));

      connect(printShortcuts, SIGNAL(clicked()), SLOT(printShortcutsClicked()));

      recordButtons = new QButtonGroup(this);
      recordButtons->setExclusive(false);
      recordButtons->addButton(recordRewind, RMIDI_REWIND);
      recordButtons->addButton(recordTogglePlay,   RMIDI_TOGGLE_PLAY);
      recordButtons->addButton(recordPlay,   RMIDI_PLAY);
      recordButtons->addButton(recordStop,   RMIDI_STOP);
      recordButtons->addButton(rcr2,         RMIDI_NOTE1);
      recordButtons->addButton(rcr3,         RMIDI_NOTE2);
      recordButtons->addButton(rcr4,         RMIDI_NOTE4);
      recordButtons->addButton(rcr5,         RMIDI_NOTE8);
      recordButtons->addButton(rcr6,         RMIDI_NOTE16);
      recordButtons->addButton(rcr7,         RMIDI_NOTE32);
      recordButtons->addButton(rcr8,         RMIDI_NOTE64);
      recordButtons->addButton(rcr9,         RMIDI_REST);
      recordButtons->addButton(rcr10,        RMIDI_DOT);
      recordButtons->addButton(rcr11,        RMIDI_DOTDOT);
      recordButtons->addButton(rcr12,        RMIDI_TIE);
      recordButtons->addButton(recordEditMode, RMIDI_NOTE_EDIT_MODE);

      int n = sizeof(exportAudioSampleRates)/sizeof(*exportAudioSampleRates);
      exportAudioSampleRate->clear();
      for (int idx = 0; idx < n; ++idx)
            exportAudioSampleRate->addItem(QString("%1").arg(exportAudioSampleRates[idx]));

      connect(recordButtons,          SIGNAL(buttonClicked(int)), SLOT(recordButtonClicked(int)));
      connect(midiRemoteControlClear, SIGNAL(clicked()), SLOT(midiRemoteControlClearClicked()));
      connect(sfOpenButton,           SIGNAL(clicked()), SLOT(selectSoundFont()));
      updateRemote();
      }

//---------------------------------------------------------
//   setPreferences
//---------------------------------------------------------

void PreferenceDialog::setPreferences(const Preferences& p)
      {
      prefs = p;
      updateValues();
      }

//---------------------------------------------------------
//   ~PreferenceDialog
//---------------------------------------------------------

PreferenceDialog::~PreferenceDialog()
      {
      qDeleteAll(localShortcuts);
      }

//---------------------------------------------------------
//   recordButtonClicked
//---------------------------------------------------------

void PreferenceDialog::recordButtonClicked(int val)
      {
      foreach(QAbstractButton* b, recordButtons->buttons()) {
            b->setChecked(recordButtons->id(b) == val);
            }
      mscore->setMidiRecordId(val);
      }

//---------------------------------------------------------
//   updateRemote
//---------------------------------------------------------

void PreferenceDialog::updateRemote()
      {
      rewindActive->setChecked(preferences.midiRemote[RMIDI_REWIND].type != -1);
      togglePlayActive->setChecked(preferences.midiRemote[RMIDI_TOGGLE_PLAY].type   != -1);
      playActive->setChecked(preferences.midiRemote[RMIDI_PLAY].type         != -1);
      stopActive->setChecked(preferences.midiRemote[RMIDI_STOP].type         != -1);
      rca2->setChecked(preferences.midiRemote[RMIDI_NOTE1].type        != -1);
      rca3->setChecked(preferences.midiRemote[RMIDI_NOTE2].type        != -1);
      rca4->setChecked(preferences.midiRemote[RMIDI_NOTE4].type        != -1);
      rca5->setChecked(preferences.midiRemote[RMIDI_NOTE8].type        != -1);
      rca6->setChecked(preferences.midiRemote[RMIDI_NOTE16].type       != -1);
      rca7->setChecked(preferences.midiRemote[RMIDI_NOTE32].type       != -1);
      rca8->setChecked(preferences.midiRemote[RMIDI_NOTE64].type      != -1);
      rca9->setChecked(preferences.midiRemote[RMIDI_REST].type        != -1);
      rca10->setChecked(preferences.midiRemote[RMIDI_DOT].type         != -1);
      rca11->setChecked(preferences.midiRemote[RMIDI_DOTDOT].type      != -1);
      rca12->setChecked(preferences.midiRemote[RMIDI_TIE].type        != -1);
      editModeActive->setChecked(preferences.midiRemote[RMIDI_NOTE_EDIT_MODE].type != -1);

      int id = mscore->midiRecordId();
      recordRewind->setChecked(id == RMIDI_REWIND);
      recordTogglePlay->setChecked(id == RMIDI_TOGGLE_PLAY);
      recordPlay->setChecked(id == RMIDI_PLAY);
      recordStop->setChecked(id == RMIDI_STOP);
      rcr2->setChecked(id       == RMIDI_NOTE1);
      rcr3->setChecked(id       == RMIDI_NOTE2);
      rcr4->setChecked(id       == RMIDI_NOTE4);
      rcr5->setChecked(id       == RMIDI_NOTE8);
      rcr6->setChecked(id       == RMIDI_NOTE16);
      rcr7->setChecked(id       == RMIDI_NOTE32);
      rcr8->setChecked(id       == RMIDI_NOTE64);
      rcr9->setChecked(id       == RMIDI_REST);
      rcr10->setChecked(id      == RMIDI_DOT);
      rcr11->setChecked(id      == RMIDI_DOTDOT);
      rcr12->setChecked(id      == RMIDI_TIE);
      recordEditMode->setChecked(id == RMIDI_NOTE_EDIT_MODE);
      }

//---------------------------------------------------------
//   updateValues
//---------------------------------------------------------

void PreferenceDialog::updateValues()
      {
      rcGroup->setChecked(prefs.useMidiRemote);
      fgWallpaper->setText(prefs.fgWallpaper);
      bgWallpaper->setText(prefs.bgWallpaper);

      bgColorButton->setChecked(prefs.bgUseColor);
      bgWallpaperButton->setChecked(!prefs.bgUseColor);
      fgColorButton->setChecked(prefs.fgUseColor);
      fgWallpaperButton->setChecked(!prefs.fgUseColor);

      if (prefs.bgUseColor) {
            bgColorLabel->setColor(MScore::bgColor);
            bgColorLabel->setPixmap(0);
            }
      else {
            bgColorLabel->setPixmap(new QPixmap(bgWallpaper->text()));
            }

      if (prefs.fgUseColor) {
            fgColorLabel->setColor(prefs.fgColor);
            fgColorLabel->setPixmap(0);
            }
      else {
            fgColorLabel->setPixmap(new QPixmap(fgWallpaper->text()));
            }

      iconWidth->setValue(prefs.iconWidth);
      iconHeight->setValue(prefs.iconHeight);

      replaceFractions->setChecked(MScore::replaceFractions);
      replaceCopyrightSymbol->setChecked(prefs.replaceCopyrightSymbol);

      enableMidiInput->setChecked(prefs.enableMidiInput);
      playNotes->setChecked(prefs.playNotes);

      //Update
      checkUpdateStartup->clear();
      int curPeriodIdx = 0;

      for(unsigned i = 0; i < sizeof(updatePeriods)/sizeof(*updatePeriods); ++i) {
            checkUpdateStartup->addItem(qApp->translate("preferences", updatePeriods[i].text), i);
            if (updatePeriods[i].time == prefs.checkUpdateStartup)
                  curPeriodIdx = i;
            }
      checkUpdateStartup->setCurrentIndex(curPeriodIdx);

      if (seq->isRunning()) {
            QList<QString> sl = seq->inputPorts();
            int idx = 0;
            for (QList<QString>::iterator i = sl.begin(); i != sl.end(); ++i, ++idx) {
                  jackRPort->addItem(*i);
                  jackLPort->addItem(*i);
                  if (prefs.rPort == *i)
                        jackRPort->setCurrentIndex(idx);
                  if (prefs.lPort == *i)
                        jackLPort->setCurrentIndex(idx);
                  }
            }
      else {
            jackRPort->setEnabled(false);
            jackLPort->setEnabled(false);
            }

      soundFont->setText(MScore::soundFont);
      navigatorShow->setChecked(prefs.showNavigator);
      playPanelShow->setChecked(prefs.showPlayPanel);
      webPanelShow->setChecked(prefs.showWebPanel);

      alsaDriver->setChecked(prefs.useAlsaAudio);
      jackDriver->setChecked(prefs.useJackAudio);
      portaudioDriver->setChecked(prefs.usePortaudioAudio);
      pulseaudioDriver->setChecked(prefs.usePulseAudio);
      useJackMidi->setChecked(prefs.useJackMidi);

      alsaDevice->setText(prefs.alsaDevice);

      int index = alsaSampleRate->findText(QString("%1").arg(prefs.alsaSampleRate));
      alsaSampleRate->setCurrentIndex(index);
      index = alsaPeriodSize->findText(QString("%1").arg(prefs.alsaPeriodSize));
      alsaPeriodSize->setCurrentIndex(index);

      alsaFragments->setValue(prefs.alsaFragments);
      drawAntialiased->setChecked(prefs.antialiasedDrawing);
      switch(prefs.sessionStart) {
            case EMPTY_SESSION:  emptySession->setChecked(true); break;
            case LAST_SESSION:   lastSession->setChecked(true); break;
            case NEW_SESSION:    newSession->setChecked(true); break;
            case SCORE_SESSION:  scoreSession->setChecked(true); break;
            }
      sessionScore->setText(prefs.startScore);
      showSplashScreen->setChecked(prefs.showSplashScreen);
      expandRepeats->setChecked(prefs.midiExpandRepeats);
      instrumentList1->setText(prefs.instrumentList1);
      instrumentList2->setText(prefs.instrumentList2);

      importLayout->setChecked(prefs.musicxmlImportLayout);
      importBreaks->setChecked(prefs.musicxmlImportBreaks);
      exportLayout->setChecked(prefs.musicxmlExportLayout);
      switch(prefs.musicxmlExportBreaks) {
            case ALL_BREAKS:     exportAllBreaks->setChecked(true); break;
            case MANUAL_BREAKS:  exportManualBreaks->setChecked(true); break;
            case NO_BREAKS:      exportNoBreaks->setChecked(true); break;
            }

      midiPorts->setValue(prefs.midiPorts);
      rememberLastMidiConnections->setChecked(prefs.rememberLastMidiConnections);
      proximity->setValue(prefs.proximity);
      autoSave->setChecked(prefs.autoSave);
      autoSaveTime->setValue(prefs.autoSaveTime);
      pngResolution->setValue(prefs.pngResolution);
      pngTransparent->setChecked(prefs.pngTransparent);
      for (int i = 0; i < language->count(); ++i) {
            if (language->itemText(i).startsWith(prefs.language)) {
                  language->setCurrentIndex(i);
                  break;
                  }
            }
      //
      // initialize local shortcut table
      //    we need a deep copy to be able to rewind all
      //    changes on "Abort"
      //
      qDeleteAll(localShortcuts);
      localShortcuts.clear();
      foreach(const Shortcut* s, Shortcut::shortcuts())
            localShortcuts[s->key()] = new Shortcut(*s);
      updateSCListView();

      //
      // initialize portaudio
      //
#ifdef USE_PORTAUDIO
      if (usePortaudio) {
            Portaudio* audio = static_cast<Portaudio*>(seq->getDriver());
            if (audio) {
                  QStringList apis = audio->apiList();
                  portaudioApi->addItems(apis);
                  portaudioApi->setCurrentIndex(audio->currentApi());

                  QStringList devices = audio->deviceList(audio->currentApi());
                  portaudioDevice->addItems(devices);
                  portaudioDevice->setCurrentIndex(audio->currentDevice());

                  connect(portaudioApi, SIGNAL(activated(int)), SLOT(portaudioApiActivated(int)));
#ifdef USE_PORTMIDI
                  PortMidiDriver* midiDriver = static_cast<PortMidiDriver*>(audio->mididriver());
                  if(midiDriver){
                        QStringList midiInputs = midiDriver->deviceInList();
                        int curMidiInIdx = 0;
                        for(int i = 0; i < midiInputs.size(); ++i) {
                              portMidiInput->addItem(midiInputs.at(i), i);
                              if (midiInputs.at(i) == prefs.portMidiInput)
                                    curMidiInIdx = i;
                              }
                        portMidiInput->setCurrentIndex(curMidiInIdx);
                        }
#endif
                  }
            }
#endif

#ifndef HAS_MIDI
      enableMidiInput->setEnabled(false);
      rcGroup->setEnabled(false);
#endif
      //
      // score settings
      //
      scale->setValue(prefs.mag);

      defaultPlayDuration->setValue(MScore::defaultPlayDuration);
      importStyleFile->setText(prefs.importStyleFile);
      int shortestNoteIndex = 2;
      int nn = (prefs.shortestNote * 16)/MScore::division;
      switch(nn) {
            case 16: shortestNoteIndex = 0; break;
            case 8:  shortestNoteIndex = 1; break;
            case 4:  shortestNoteIndex = 2; break;
            case 2:  shortestNoteIndex = 3; break;
            case 1:  shortestNoteIndex = 4; break;
            }
      shortestNote->setCurrentIndex(shortestNoteIndex);
      useImportBuildinStyle->setChecked(prefs.importStyleFile.isEmpty());
      useImportStyleFile->setChecked(!prefs.importStyleFile.isEmpty());

      importCharsetList->clear();
      QList<QByteArray> charsets = QTextCodec::availableCodecs();
      qSort(charsets.begin(), charsets.end());
      int idx = 0;
      foreach (QByteArray charset, charsets) {
            importCharsetList->addItem(charset);
            if (charset == prefs.importCharset)
                  importCharsetList->setCurrentIndex(idx);
            idx++;
            }

      warnPitchRange->setChecked(MScore::warnPitchRange);

      language->clear();
      int curIdx = 0;
      for(int i = 0; i < mscore->languages().size(); ++i) {
            language->addItem(mscore->languages().at(i).name, i);
            if (mscore->languages().at(i).key == prefs.language)
                  curIdx = i;
            }
      language->setCurrentIndex(curIdx);

      oscServer->setChecked(prefs.useOsc);
      oscPort->setValue(prefs.oscPort);

      styleName->setCurrentIndex(prefs.globalStyle);

      defaultStyle->setText(prefs.defaultStyleFile);

      myScores->setText(prefs.myScoresPath);
      myStyles->setText(prefs.myStylesPath);
      myImages->setText(prefs.myImagesPath);
      myTemplates->setText(prefs.myTemplatesPath);
      myPlugins->setText(prefs.myPluginsPath);
      mySoundFonts->setText(prefs.mySoundFontsPath);

      nativeDialogs->setChecked(prefs.nativeDialogs);
      idx = 0;
      int n = sizeof(exportAudioSampleRates)/sizeof(*exportAudioSampleRates);
      for (;idx < n; ++idx) {
            if (exportAudioSampleRates[idx] == prefs.exportAudioSampleRate)
                  break;
            }
      if (idx == n)     // if not found in table
            idx = 0;
      exportAudioSampleRate->setCurrentIndex(idx);

      sfChanged = false;
      }

//---------------------------------------------------------
//   portaudioApiActivated
//---------------------------------------------------------

#ifdef USE_PORTAUDIO
void PreferenceDialog::portaudioApiActivated(int idx)
      {
      Portaudio* audio = static_cast<Portaudio*>(seq->getDriver());
      QStringList devices = audio->deviceList(idx);
      portaudioDevice->clear();
      portaudioDevice->addItems(devices);
      }
#else
void PreferenceDialog::portaudioApiActivated(int)  {}
#endif

//---------------------------------------------------------
//   ShortcutITem
//---------------------------------------------------------

bool ShortcutItem::operator<(const QTreeWidgetItem& item) const
      {

      const QTreeWidget * pTree =treeWidget ();
      int column   = pTree ? pTree->sortColumn() : 0;
      return QString::localeAwareCompare(text(column).toLower(), item.text(column).toLower()) > 0;
      }

//---------------------------------------------------------
//   updateSCListView
//---------------------------------------------------------

void PreferenceDialog::updateSCListView()
      {
      shortcutList->clear();
      foreach (Shortcut* s, localShortcuts) {
            if (!s)
                  continue;
            ShortcutItem* newItem = new ShortcutItem;
            newItem->setText(0, s->descr());
            if (s->icon() != -1)
                  newItem->setIcon(0, *icons[s->icon()]);
            newItem->setText(1, s->keysToString());
            newItem->setData(0, Qt::UserRole, s->key());
            shortcutList->addTopLevelItem(newItem);
            }
      shortcutList->resizeColumnToContents(0);
      }

//---------------------------------------------------------
//   resetShortcutClicked
//    reset selected shortcut to buildin default
//---------------------------------------------------------

void PreferenceDialog::resetShortcutClicked()
      {
      QTreeWidgetItem* active = shortcutList->currentItem();
      if (!active)
            return;
      QString str = active->data(0, Qt::UserRole).toString();
      if (str.isEmpty())
            return;
      Shortcut* shortcut = localShortcuts[str.toLatin1().data()];

      shortcut->reset();

      active->setText(1, shortcut->keysToString());
      shortcutsChanged = true;
      }

//---------------------------------------------------------
//   clearShortcutClicked
//---------------------------------------------------------

void PreferenceDialog::clearShortcutClicked()
      {
      QTreeWidgetItem* active = shortcutList->currentItem();
      if (!active)
            return;
      QString str = active->data(0, Qt::UserRole).toString();
      if (str.isEmpty())
            return;
      Shortcut* s = localShortcuts[str.toLatin1().data()];
      s->clear();
      active->setText(1, "");
      shortcutsChanged = true;
      }

//---------------------------------------------------------
//   selectFgWallpaper
//---------------------------------------------------------

void PreferenceDialog::selectFgWallpaper()
      {
      QString s = mscore->getWallpaper(tr("Choose Notepaper"));
      if (!s.isNull())
            fgWallpaper->setText(s);
      }

//---------------------------------------------------------
//   selectBgWallpaper
//---------------------------------------------------------

void PreferenceDialog::selectBgWallpaper()
      {
      QString s = mscore->getWallpaper(tr("Choose Background Wallpaper"));
      if (!s.isNull())
            bgWallpaper->setText(s);
      }

//---------------------------------------------------------
//   selectDefaultStyle
//---------------------------------------------------------

void PreferenceDialog::selectDefaultStyle()
      {
      QString s = mscore->getStyleFilename(true, tr("Choose Default Style"));
      if (!s.isNull())
            defaultStyle->setText(s);
      }

//---------------------------------------------------------
//   selectPartStyle
//---------------------------------------------------------

void PreferenceDialog::selectPartStyle()
      {
      QString s = mscore->getStyleFilename(true, tr("Choose Default Style for Parts"));
      if (!s.isNull())
            partStyle->setText(s);
      }

//---------------------------------------------------------
//   selectInstrumentList1
//---------------------------------------------------------

void PreferenceDialog::selectInstrumentList1()
      {
      QString s = QFileDialog::getOpenFileName(
         this,
         tr("Choose Instrument List"),
         instrumentList1->text(),
         tr("Instrument List (*.xml)")
         );
      if (!s.isNull())
            instrumentList1->setText(s);
      }

//---------------------------------------------------------
//   selectInstrumentList2
//---------------------------------------------------------

void PreferenceDialog::selectInstrumentList2()
      {
      QString s = QFileDialog::getOpenFileName(
         this,
         tr("Choose Instrument List"),
         instrumentList2->text(),
         tr("Instrument List (*.xml)")
         );
      if (!s.isNull())
            instrumentList2->setText(s);
      }

//---------------------------------------------------------
//   selectStartWith
//---------------------------------------------------------

void PreferenceDialog::selectStartWith()
      {
      QString s = QFileDialog::getOpenFileName(
         this,
         tr("Choose Starting Score"),
         sessionScore->text(),
         tr("MuseScore Files (*.mscz *.mscx *.msc);;All (*)")
         );
      if (!s.isNull())
            sessionScore->setText(s);
      }

//---------------------------------------------------------
//   fgClicked
//---------------------------------------------------------

void PreferenceDialog::fgClicked(bool id)
      {
      fgWallpaper->setEnabled(!id);
      fgWallpaperSelect->setEnabled(!id);

      if (id) {
            // fgColorLabel->setColor(p->fgColor);
            fgColorLabel->setPixmap(0);
            }
      else {
            fgColorLabel->setPixmap(new QPixmap(fgWallpaper->text()));
            }
      }

//---------------------------------------------------------
//   bgClicked
//---------------------------------------------------------

void PreferenceDialog::bgClicked(bool id)
      {
      bgWallpaper->setEnabled(!id);
      bgWallpaperSelect->setEnabled(!id);

      if (id) {
            // bgColorLabel->setColor(p->bgColor);
            bgColorLabel->setPixmap(0);
            }
      else {
            bgColorLabel->setPixmap(new QPixmap(bgWallpaper->text()));
            }
      }

//---------------------------------------------------------
//   buttonBoxClicked
//---------------------------------------------------------

void PreferenceDialog::buttonBoxClicked(QAbstractButton* button)
      {
      switch(buttonBox->standardButton(button)) {
            case QDialogButtonBox::Apply:
                  apply();
                  break;
            case QDialogButtonBox::Ok:
                  apply();
            case QDialogButtonBox::Cancel:
            default:
                  hide();
                  break;
            }
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void PreferenceDialog::apply()
      {
      prefs.useMidiRemote  = rcGroup->isChecked();
      prefs.fgWallpaper    = fgWallpaper->text();
      prefs.bgWallpaper    = bgWallpaper->text();
      prefs.fgColor        = fgColorLabel->color();
      MScore::bgColor            = bgColorLabel->color();

      prefs.iconWidth      = iconWidth->value();
      prefs.iconHeight     = iconHeight->value();

      prefs.bgUseColor     = bgColorButton->isChecked();
      prefs.fgUseColor     = fgColorButton->isChecked();
      prefs.enableMidiInput = enableMidiInput->isChecked();
      prefs.playNotes      = playNotes->isChecked();
      if (prefs.lPort != jackLPort->currentText()
         || prefs.rPort != jackRPort->currentText()) {
            // TODO: change ports
            prefs.lPort = jackLPort->currentText();
            prefs.rPort = jackRPort->currentText();
            }
      MScore::soundFont              = soundFont->text();
      prefs.showNavigator      = navigatorShow->isChecked();
      prefs.showPlayPanel      = playPanelShow->isChecked();
      prefs.showWebPanel       = webPanelShow->isChecked();
      prefs.antialiasedDrawing = drawAntialiased->isChecked();

      if (
         (prefs.useAlsaAudio != alsaDriver->isChecked())
         || (prefs.useJackAudio != jackDriver->isChecked())
         || (prefs.usePortaudioAudio != portaudioDriver->isChecked())
         || (prefs.usePulseAudio != pulseaudioDriver->isChecked())
         || (prefs.useJackMidi != useJackMidi->isChecked())
         || (prefs.alsaDevice != alsaDevice->text())
         || (prefs.alsaSampleRate != alsaSampleRate->currentText().toInt())
         || (prefs.alsaPeriodSize != alsaPeriodSize->currentText().toInt())
         || (prefs.alsaFragments != alsaFragments->value())
            ) {
            seq->exit();
            prefs.useAlsaAudio       = alsaDriver->isChecked();
            prefs.useJackAudio       = jackDriver->isChecked();
            prefs.usePortaudioAudio  = portaudioDriver->isChecked();
            prefs.usePulseAudio      = pulseaudioDriver->isChecked();
            prefs.useJackMidi        = useJackMidi->isChecked();
            prefs.alsaDevice         = alsaDevice->text();
            prefs.alsaSampleRate     = alsaSampleRate->currentText().toInt();
            prefs.alsaPeriodSize     = alsaPeriodSize->currentText().toInt();
            prefs.alsaFragments      = alsaFragments->value();
            if (!seq->init()) {
                  qDebug("sequencer init failed\n");
                  }
            }

#ifdef USE_PORTAUDIO
      Portaudio* audio = static_cast<Portaudio*>(seq->getDriver());
      prefs.portaudioDevice = audio->deviceIndex(portaudioApi->currentIndex(),
         portaudioDevice->currentIndex());
#endif

#ifdef USE_PORTMIDI
      prefs.portMidiInput = portMidiInput->currentText();
#endif

      if (lastSession->isChecked())
            prefs.sessionStart = LAST_SESSION;
      else if (newSession->isChecked())
            prefs.sessionStart = NEW_SESSION;
      else if (scoreSession->isChecked())
            prefs.sessionStart = SCORE_SESSION;
      else if (emptySession->isChecked())
            prefs.sessionStart = EMPTY_SESSION;
      prefs.startScore         = sessionScore->text();
      prefs.myScoresPath       = myScores->text();
      prefs.myStylesPath       = myStyles->text();
      prefs.myImagesPath       = myImages->text();
      prefs.myTemplatesPath    = myTemplates->text();
      prefs.myPluginsPath      = myPlugins->text();
      prefs.mySoundFontsPath   = mySoundFonts->text();

      prefs.nativeDialogs      = nativeDialogs->isChecked();
      int idx = exportAudioSampleRate->currentIndex();
      prefs.exportAudioSampleRate = exportAudioSampleRates[idx];

      prefs.showSplashScreen   = showSplashScreen->isChecked();
      prefs.midiExpandRepeats  = expandRepeats->isChecked();
      prefs.instrumentList1    = instrumentList1->text();
      prefs.instrumentList2    = instrumentList2->text();

      prefs.musicxmlImportLayout  = importLayout->isChecked();
      prefs.musicxmlImportBreaks  = importBreaks->isChecked();
      prefs.musicxmlExportLayout  = exportLayout->isChecked();
      if (exportAllBreaks->isChecked())
            prefs.musicxmlExportBreaks = ALL_BREAKS;
      else if (exportManualBreaks->isChecked())
            prefs.musicxmlExportBreaks = MANUAL_BREAKS;
      else if (exportNoBreaks->isChecked())
            prefs.musicxmlExportBreaks = NO_BREAKS;

      prefs.midiPorts          = midiPorts->value();
      prefs.rememberLastMidiConnections = rememberLastMidiConnections->isChecked();
      prefs.proximity          = proximity->value();
      prefs.autoSave           = autoSave->isChecked();
      prefs.autoSaveTime       = autoSaveTime->value();
      prefs.pngResolution      = pngResolution->value();
      prefs.pngTransparent     = pngTransparent->isChecked();
      converterDpi             = prefs.pngResolution;

      if (shortcutsChanged) {
            shortcutsChanged = false;
            foreach(const Shortcut* s, localShortcuts) {
                  Shortcut* os = Shortcut::getShortcut(s->key());
                  if (os) {
                        if (!os->compareKeys(*s))
                              os->setKeys(s->keys());
                        }
                  }
            Shortcut::dirty = true;
            }

      int lang = language->itemData(language->currentIndex()).toInt();
      QString l = lang == 0 ? "system" : mscore->languages().at(lang).key;
      bool languageChanged = l != prefs.language;
      prefs.language = l;

      MScore::replaceFractions       = replaceFractions->isChecked();
      prefs.replaceCopyrightSymbol = replaceCopyrightSymbol->isChecked();

      //update
      int periodIndex = checkUpdateStartup->currentIndex();
      int t = updatePeriods[periodIndex].time;
      prefs.checkUpdateStartup = t;

      prefs.mag         = scale->value();

      MScore::defaultPlayDuration = defaultPlayDuration->value();

      if (useImportStyleFile->isChecked())
            prefs.importStyleFile = importStyleFile->text();
      else
            prefs.importStyleFile.clear();

      int ticks = MScore::division/4;
      switch(shortestNote->currentIndex()) {
            case 0: ticks = MScore::division;    break;
            case 1: ticks = MScore::division/2;  break;
            case 2: ticks = MScore::division/4;  break;
            case 3: ticks = MScore::division/8;  break;
            case 4: ticks = MScore::division/16; break;
            }
      prefs.shortestNote = ticks;

      prefs.importCharset = importCharsetList->currentText();
      MScore::warnPitchRange = warnPitchRange->isChecked();

      prefs.useOsc  = oscServer->isChecked();
      prefs.oscPort = oscPort->value();
      if (styleName->currentIndex() == STYLE_DARK) {
            iconGroup = "icons-dark/";
            appStyleFile = ":/data/appstyle-dark.css";
            prefs.styleName = "dark";
            prefs.globalStyle = STYLE_DARK;
            }
      else {
            iconGroup = "icons/";
            appStyleFile = ":/data/appstyle.css";
            prefs.styleName = "native";
            prefs.globalStyle = STYLE_NATIVE;
            }

      if (languageChanged) {
            setMscoreLocale(prefs.language);
            mscore->update();
            }
      if (defaultStyle->text() != prefs.defaultStyleFile) {
            prefs.defaultStyleFile = defaultStyle->text();
            prefs.readDefaultStyle();
            }

      qApp->setStyleSheet(appStyleSheet());
      genIcons();

      mscore->setIconSize(QSize(preferences.iconWidth, preferences.iconHeight));

      preferences = prefs;
      emit preferencesChanged();
      preferences.write();
      mscore->startAutoSave();
      }

//---------------------------------------------------------
//   readDefaultStyle
//---------------------------------------------------------

bool Preferences::readDefaultStyle()
      {
      if (defaultStyleFile.isEmpty())
            return false;
      MStyle* style = new MStyle;
      QFile f(defaultStyleFile);
      if (!f.open(QIODevice::ReadOnly))
            return false;
      bool rv = style->load(&f);
      if (rv)
            MScore::setDefaultStyle(style);     // transfer ownership
      f.close();
      return rv;
      }

//---------------------------------------------------------
//   resetAllValues
//---------------------------------------------------------

void PreferenceDialog::resetAllValues()
      {
      prefs.init();
      updateValues();

      shortcutsChanged = true;
      qDeleteAll(localShortcuts);
      localShortcuts.clear();
      Shortcut::resetToDefault();
      foreach(const Shortcut* s, Shortcut::shortcuts())
            localShortcuts[s->key()] = new Shortcut(*s);
      updateSCListView();
      }

//---------------------------------------------------------
//   styleFileButtonClicked
//---------------------------------------------------------

void PreferenceDialog::styleFileButtonClicked()
      {
      QString fn = mscore->getStyleFilename(true);
      if (fn.isEmpty())
            return;
      importStyleFile->setText(fn);
      }

//---------------------------------------------------------
//   midiRemoteControlClearClicked
//---------------------------------------------------------

void PreferenceDialog::midiRemoteControlClearClicked()
      {
      for (int i = 0; i < MIDI_REMOTES; ++i)
            preferences.midiRemote[i].type = MIDI_REMOTE_TYPE_INACTIVE;
      updateRemote();
      }

//---------------------------------------------------------
//   selectSoundFont
//---------------------------------------------------------

void PreferenceDialog::selectSoundFont()
      {
      QStringList s = mscore->getSoundFont(soundFont->text());
      if(s.size() > 0)
            soundFont->setText(s.front());
      }

//---------------------------------------------------------
//   selectScoresDirectory
//---------------------------------------------------------

void PreferenceDialog::selectScoresDirectory()
      {
      QString s = QFileDialog::getExistingDirectory(
         this,
         tr("Choose Scores Directory"),
         myScores->text()
         );
      if (!s.isNull())
            myScores->setText(s);
      }

//---------------------------------------------------------
//   selectStylesDirectory
//---------------------------------------------------------

void PreferenceDialog::selectStylesDirectory()
      {
      QString s = QFileDialog::getExistingDirectory(
         this,
         tr("Choose Styles Directory"),
         myStyles->text()
         );
      if (!s.isNull())
            myStyles->setText(s);
      }

//---------------------------------------------------------
//   selectTemplatesDirectory
//---------------------------------------------------------

void PreferenceDialog::selectTemplatesDirectory()
      {
      QString s = QFileDialog::getExistingDirectory(
         this,
         tr("Choose Templates Directory"),
         myTemplates->text()
         );
      if (!s.isNull())
            myTemplates->setText(s);
      }

//---------------------------------------------------------
//   selectPluginsDirectory
//---------------------------------------------------------

void PreferenceDialog::selectPluginsDirectory()
      {
      QString s = QFileDialog::getExistingDirectory(
         this,
         tr("Choose Plugins Directory"),
         myPlugins->text()
         );
      if (!s.isNull())
            myPlugins->setText(s);
      }

//---------------------------------------------------------
//   selectSoundFontsDirectory
//---------------------------------------------------------

void PreferenceDialog::selectSoundFontsDirectory()
      {
      QString s = QFileDialog::getExistingDirectory(
         this,
         tr("Choose SoundFonts Directory"),
         mySoundFonts->text()
         );
      if (!s.isNull())
            mySoundFonts->setText(s);
      }

//---------------------------------------------------------
//   selectImagesDirectory
//---------------------------------------------------------

void PreferenceDialog::selectImagesDirectory()
      {
      QString s = QFileDialog::getExistingDirectory(
         this,
         tr("Choose Images Directory"),
         myImages->text()
         );
      if (!s.isNull())
            myImages->setText(s);
      }

//---------------------------------------------------------
//   defineShortcutClicked
//---------------------------------------------------------

void PreferenceDialog::defineShortcutClicked()
      {
      QTreeWidgetItem* active = shortcutList->currentItem();
      if (!active)
            return;
      QString str = active->data(0, Qt::UserRole).toString();
      if (str.isEmpty())
            return;
      Shortcut* s = localShortcuts[str.toLatin1().data()];
      ShortcutCaptureDialog sc(s, localShortcuts, this);
      int rv = sc.exec();
      if (rv == 0)
            return;
      if (rv == 2)
            s->clear();
      s->addShortcut(sc.getKey());
      active->setText(1, s->keysToString());
      shortcutsChanged = true;
      }

//---------------------------------------------------------
//   readPluginList
//---------------------------------------------------------

bool Preferences::readPluginList()
      {
      QFile f(dataPath + "/plugins.xml");
      if (!f.exists())
            return false;
      if (!f.open(QIODevice::ReadOnly)) {
            qDebug("cannot open plugins file <%s>\n", qPrintable(f.fileName()));
            return false;
            }
      XmlReader e(&f);
      while (e.readNextStartElement()) {
            if (e.name() == "museScore") {
                  while (e.readNextStartElement()) {
                        const QStringRef& tag(e.name());
                        if (tag == "Plugin") {
                              PluginDescription d;
                              while (e.readNextStartElement()) {
                                    const QStringRef& tag(e.name());
                                    if (tag == "path")
                                          d.path = e.readElementText();
                                    else if (tag == "load")
                                          d.load = e.readInt();
                                    else if (tag == "SC")
                                          d.shortcut.read(e);
                                    else if (tag == "version")
                                          d.version = e.readElementText();
                                    else if (tag == "description")
                                          d.description = e.readElementText();
                                    else
                                          e.unknown();
                                    }
                              if (d.path.endsWith(".qml"))
                                    pluginList.append(d);
                              }
                        else
                              e.unknown();
                        }
                  }
            else
                  e.unknown();
            }
      return true;
      }


//---------------------------------------------------------
//   writePluginList
//---------------------------------------------------------

void Preferences::writePluginList()
      {
      QDir dir;
      dir.mkpath(dataPath);
      QFile f(dataPath + "/plugins.xml");
      if (!f.open(QIODevice::WriteOnly)) {
            qDebug("cannot create plugin file <%s>\n", qPrintable(f.fileName()));
            return;
            }
      Xml xml(&f);
      xml.header();
      xml.stag("museScore version=\"" MSC_VERSION "\"");
      foreach(const PluginDescription& d, pluginList) {
            xml.stag("Plugin");
            xml.tag("path", d.path);
            xml.tag("load", d.load);
            xml.tag("version", d.version);
            xml.tag("description", d.description);
            if (!d.shortcut.keys().isEmpty())
                  d.shortcut.write(xml);
            xml.etag();
            }
      xml.etag();
      f.close();
      }

//---------------------------------------------------------
//   updatePluginList
//    scan plugin folders for new plugins and update
//    pluginList
//---------------------------------------------------------

static void updatePluginList(QList<QString>& pluginPathList, const QString& pluginPath,
   QList<PluginDescription>& pluginList)
      {
      QDirIterator it(pluginPath, QDir::NoDot|QDir::NoDotDot|QDir::Dirs|QDir::Files,
         QDirIterator::Subdirectories);
      while (it.hasNext()) {
            it.next();
            QFileInfo fi = it.fileInfo();
            QString path(fi.absoluteFilePath());
            printf("   <%s>\n", qPrintable(path));
            if (fi.isFile()) {
                  if (path.endsWith(".qml")) {
                        bool alreadyInList = false;
                        foreach (const PluginDescription& p, pluginList) {
                              if (p.path == path) {
                                    alreadyInList = true;
                                    break;
                                    }
                              }
                        if (!alreadyInList) {
                              PluginDescription p;
                              p.path = path;
                              p.load = false;
                              collectPluginMetaInformation(&p);
                              pluginList.append(p);
                              }
                        }
                  }
            else
                  updatePluginList(pluginPathList, path, pluginList);
            }
      }

void Preferences::updatePluginList()
      {
      pluginList.clear();
      QList<QString> pluginPathList;
      pluginPathList.append(dataPath + "/plugins");
      pluginPathList.append(mscoreGlobalShare + "plugins");
      pluginPathList.append(myPluginsPath);

      foreach(QString pluginPath, pluginPathList) {
            ::updatePluginList(pluginPathList, pluginPath, pluginList);
            }
      }

//---------------------------------------------------------
//   printShortcutsClicked
//---------------------------------------------------------

void PreferenceDialog::printShortcutsClicked()
      {
      QPrinter printer(QPrinter::HighResolution);
      MStyle* s = MScore::defaultStyle();
      const PageFormat* pf = s->pageFormat();
      printer.setPaperSize(pf->size(), QPrinter::Inch);

      printer.setCreator("MuseScore Version: " VERSION);
      printer.setFullPage(true);
      printer.setColorMode(QPrinter::Color);
      printer.setDocName(tr("MuseScore Shortcuts"));
      printer.setDoubleSidedPrinting(pf->twosided());
      printer.setOutputFormat(QPrinter::NativeFormat);

      QPrintDialog pd(&printer, 0);
      pd.setWindowTitle(tr("Print Shortcuts"));
      if (!pd.exec())
            return;
      qreal dpmm = printer.logicalDpiY() / 25.4;
      qreal ph   = printer.height();
      qreal tm   = dpmm * 15;
      qreal bm   = dpmm * 15;
      qreal lm   = dpmm * 20;

      QPainter p;
      p.begin(&printer);
      qreal y;
      qreal lh = QFontMetricsF(p.font()).lineSpacing();

      QMapIterator<QString, Shortcut*> isc(localShortcuts);
      qreal col1Width = 0.0;
      while (isc.hasNext()) {
            isc.next();
            Shortcut* s = isc.value();
            col1Width = qMax(col1Width, QFontMetricsF(p.font()).width(s->descr()));
            }

      int idx = 0;
      isc = QMapIterator<QString, Shortcut*>(localShortcuts);
      while (isc.hasNext()) {
            isc.next();
            if (idx == 0 || y >= (ph - bm)) {
                  y = tm;
                  if (idx)
                        printer.newPage();
                  else {
                        p.save();
                        QFont font(p.font());
                        font.setPointSizeF(14.0);
                        font.setBold(true);
                        p.setFont(font);
                        p.drawText(lm, y, tr("MuseScore Shortcuts"));
                        p.restore();
                        y += QFontMetricsF(font).lineSpacing();
                        y += 5 * dpmm;
                        }
                  }
            Shortcut* s = isc.value();
            p.drawText(lm, y, s->descr());
            p.drawText(col1Width + lm + 5 * dpmm, y, s->keysToString());
            y += lh;
            ++idx;
            }
      p.end();
      }

