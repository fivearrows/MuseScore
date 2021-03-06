//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id: instrtemplate.cpp 5175 2012-01-02 08:30:52Z wschweer $
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "instrtemplate.h"
#include "xml.h"
#include "style.h"
#include "sym.h"
#include "drumset.h"
#include "clef.h"
#include "bracket.h"
#include "utils.h"
#include "tablature.h"
#include "mscore.h"

QList<InstrumentGroup*> instrumentGroups;
QList<MidiArticulation> articulation;                // global articulations

//---------------------------------------------------------
//   searchInstrumentGroup
//---------------------------------------------------------

static InstrumentGroup* searchInstrumentGroup(const QString& name)
      {
      foreach(InstrumentGroup* g, instrumentGroups) {
            if (g->id == name)
                  return g;
            }
      return 0;
      }

//---------------------------------------------------------
//   readStaffIdx
//---------------------------------------------------------

static int readStaffIdx(XmlReader& e)
      {
      int idx = e.intAttribute("staff", 1) - 1;
      if (idx >= MAX_STAVES)
            idx = MAX_STAVES-1;
      if (idx < 0)
            idx = 0;
      return idx;
      }

//---------------------------------------------------------
//   readInstrumentGroup
//---------------------------------------------------------

void InstrumentGroup::read(XmlReader& e)
      {
      id       = e.attribute("id");
      name     = e.attribute("name");
      extended = e.intAttribute("extended", 0);

      while (e.readNextStartElement()) {
            const QStringRef& tag(e.name());
            if (tag == "instrument" || tag == "Instrument") {
                  QString id = e.attribute("id");
                  InstrumentTemplate* t = searchTemplate(id);
                  if (t == 0) {
                        t = new InstrumentTemplate;
                        t->articulation.append(articulation);     // init with global articulation
                        instrumentTemplates.append(t);
                        }
                  t->read(e);
                  }
            else if (tag == "ref") {
                  InstrumentTemplate* ttt = searchTemplate(e.readElementText());
                  if (ttt) {
                        InstrumentTemplate* t = new InstrumentTemplate(*ttt);
                        instrumentTemplates.append(t);
                        }
                  else
                        qDebug("instrument reference not found <%s>", e.text().toUtf8().data());
                  }
            else if (tag == "name")
                  name = e.readElementText();
            else if (tag == "extended")
                  extended = e.readInt();
            else
                  e.unknown();
            }
      if (id.isEmpty())
            id = name.toLower().replace(" ", "-");
      }

//---------------------------------------------------------
//   InstrumentTemplate
//---------------------------------------------------------

InstrumentTemplate::InstrumentTemplate()
      {
      staves             = 1;
      minPitchA          = 0;
      maxPitchA          = 127;
      minPitchP          = 0;
      maxPitchP          = 127;
      useDrumset         = false;
      drumset            = 0;
      extended           = false;
      tablature          = 0;
      useTablature       = false;

      for (int i = 0; i < MAX_STAVES; ++i) {
            clefIdx[i]     = CLEF_G;      // CLEF_INVALID;
            staffLines[i]  = 5;           // -1;
            smallStaff[i]  = false;
            bracket[i]     = NO_BRACKET;
            bracketSpan[i] = 0;
            barlineSpan[i] = 0;
            }
      transpose.diatonic   = 0;
      transpose.chromatic  = 0;
      }

InstrumentTemplate::InstrumentTemplate(const InstrumentTemplate& t)
      {
      init(t);
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void InstrumentTemplate::init(const InstrumentTemplate& t)
      {
      trackName  = t.trackName;
      longNames  = t.longNames;
      shortNames = t.shortNames;
      staves     = t.staves;
      extended   = t.extended;

      for (int i = 0; i < MAX_STAVES; ++i) {
            clefIdx[i]     = t.clefIdx[i];
            staffLines[i]  = t.staffLines[i];
            smallStaff[i]  = t.smallStaff[i];
            bracket[i]     = t.bracket[i];
            bracketSpan[i] = t.bracketSpan[i];
            barlineSpan[i] = t.barlineSpan[i];
            }
      minPitchA  = t.minPitchA;
      maxPitchA  = t.maxPitchA;
      minPitchP  = t.minPitchP;
      maxPitchP  = t.maxPitchP;
      transpose  = t.transpose;
      useDrumset = t.useDrumset;
      if (t.drumset)
            drumset = new Drumset(*t.drumset);
      else
            drumset = 0;
      if (t.tablature)
            tablature = new Tablature(*t.tablature);
      else
            tablature = 0;
      midiActions = t.midiActions;
      channel     = t.channel;
      }

InstrumentTemplate::~InstrumentTemplate()
      {
      delete drumset;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void InstrumentTemplate::write(Xml& xml) const
      {
      xml.stag(QString("Instrument id=\"%1\"").arg(id));
      foreach(StaffName sn, longNames) {
            if (sn.pos == 0)
                  xml.tag("longName", sn.name);
            else
                  xml.tag(QString("longName pos=\"%1\"").arg(sn.pos), sn.name);
            }
      foreach(StaffName sn, shortNames) {
            if (sn.pos == 0)
                  xml.tag("shortName", sn.name);
            else
                  xml.tag(QString("shortName pos=\"%1\"").arg(sn.pos), sn.name);
            }
      xml.tag("description", trackName);
      if (extended)
            xml.tag("extended", extended);
      if (tablature)
            tablature->write(xml);
      if (staves > 1)
            xml.tag("staves", staves);
      for (int i = 0; i < staves; ++i) {
            if (i)
                  xml.tag(QString("clef staff=\"%1\"").arg(i+1), clefTable[clefIdx[i]].tag);
            else
                  xml.tag("clef", clefTable[clefIdx[i]].tag);
            if (staffLines[i] != 5) {
                  if (i)
                        xml.tag(QString("stafflines staff=\"%1\"").arg(i+1), staffLines[i]);
                  else
                        xml.tag("stafflines", staffLines[i]);
                  }
            if (smallStaff[i]) {
                  if (i)
                        xml.tag(QString("smallStaff staff=\"%1\"").arg(i+1), smallStaff[i]);
                  else
                        xml.tag("smallStaff", smallStaff[i]);
                  }

            if (bracket[i] != NO_BRACKET) {
                  if (i)
                        xml.tag(QString("bracket staff=\"%1\"").arg(i+1), bracket[i]);
                  else
                        xml.tag("bracket", bracket[i]);
                  }
            if (bracketSpan[i] != 0) {
                  if (i)
                        xml.tag(QString("bracketSpan staff=\"%1\"").arg(i+1), bracketSpan[i]);
                  else
                        xml.tag("bracketSpan", bracketSpan[i]);
                  }
            if (barlineSpan[i] != 0) {
                  if (i)
                        xml.tag(QString("barlineSpan staff=\"%1\"").arg(i+1), barlineSpan[i]);
                  else
                        xml.tag("barlineSpan", barlineSpan[i]);
                  }
            }
      if (minPitchA != 0 || maxPitchA != 127)
            xml.tag("aPitchRange", QString("%1-%2").arg(int(minPitchA)).arg(int(maxPitchA)));
      if (minPitchP != 0 || maxPitchP != 127)
            xml.tag("pPitchRange", QString("%1-%2").arg(int(minPitchP)).arg(int(maxPitchP)));
      if (transpose.diatonic)
            xml.tag("transposeDiatonic", transpose.diatonic);
      if (transpose.chromatic)
            xml.tag("transposeChromatic", transpose.chromatic);
      if (useDrumset)
            xml.tag("drumset", useDrumset);
      if (drumset)
            drumset->save(xml);
      foreach(const NamedEventList& a, midiActions)
            a.write(xml, "MidiAction");
      foreach(const Channel& a, channel)
            a.write(xml);
      foreach(const MidiArticulation& ma, articulation) {
            bool isGlobal = false;
            foreach(const MidiArticulation& ga, ::articulation) {
                  if (ma == ga) {
                        isGlobal = true;
                        break;
                        }
                  }
            if (!isGlobal)
                  ma.write(xml);
            }
      xml.etag();
      }

//---------------------------------------------------------
//   write1
//    output only translatable names
//---------------------------------------------------------

void InstrumentTemplate::write1(Xml& xml) const
      {
      xml.stag(QString("Instrument id=\"%1\"").arg(id));
      foreach(StaffName sn, longNames) {
            if (sn.pos == 0)
                  xml.tag("longName", sn.name);
            else
                  xml.tag(QString("longName pos=\"%1\"").arg(sn.pos), sn.name);
            }
      foreach(StaffName sn, shortNames) {
            if (sn.pos == 0)
                  xml.tag("shortName", sn.name);
            else
                  xml.tag(QString("shortName pos=\"%1\"").arg(sn.pos), sn.name);
            }
      xml.tag("description", trackName);
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void InstrumentTemplate::read(XmlReader& e)
      {
      id = e.attribute("id");

      while (e.readNextStartElement()) {
            const QStringRef& tag(e.name());

            if (tag == "longName" || tag == "name") {               // "name" is obsolete
                  int pos = e.intAttribute("pos", 0);
                  for (QList<StaffName>::iterator i = longNames.begin(); i != longNames.end(); ++i) {
                        if((*i).pos == pos)
                              longNames.erase(i);
                        break;
                        }
                  longNames.append(StaffName(e.readElementText(), pos));
                  }
            else if (tag == "shortName" || tag == "short-name") {   // "short-name" is obsolete
                  int pos = e.intAttribute("pos", 0);
                  for (QList<StaffName>::iterator i = shortNames.begin(); i != shortNames.end(); ++i) {
                        if((*i).pos == pos)
                              shortNames.erase(i);
                        break;
                        }
                  shortNames.append(StaffName(e.readElementText(), pos));
                  }
            else if (tag == "description")
                  trackName = e.readElementText();
            else if (tag == "extended")
                  extended = e.readInt();
            else if (tag == "staves") {
                  staves = e.readInt();
                  bracketSpan[0] = staves;
                  barlineSpan[0] = staves;
                  }
            else if (tag == "clef") {
                  int idx = readStaffIdx(e);
                  QString val(e.readElementText());
                  bool ok;
                  int i = val.toInt(&ok);
                  if (!ok) {
                        ClefType ct = Clef::clefType(val);
                        clefIdx[idx] = ct;
                        }
                  else
                        clefIdx[idx] = ClefType(i);
                  }
            else if (tag == "stafflines") {
                  int idx = readStaffIdx(e);
                  staffLines[idx] = e.readInt();
                  }
            else if (tag == "smallStaff") {
                  int idx = readStaffIdx(e);
                  smallStaff[idx] = e.readInt();
                  }
            else if (tag == "bracket") {
                  int idx = readStaffIdx(e);
                  bracket[idx] = BracketType(e.readInt());
                  }
            else if (tag == "bracketSpan") {
                  int idx = readStaffIdx(e);
                  bracketSpan[idx] = e.readInt();
                  }
            else if (tag == "barlineSpan") {
                  int idx = readStaffIdx(e);
                  barlineSpan[idx] = e.readInt();
                  }
            else if (tag == "Tablature") {
                  tablature = new Tablature;
                  tablature->read(e);
                  }
            else if (tag == "aPitchRange")
                  setPitchRange(e.readElementText(), &minPitchA, &maxPitchA);
            else if (tag == "pPitchRange")
                  setPitchRange(e.readElementText(), &minPitchP, &maxPitchP);
            else if (tag == "transposition") {    // obsolete
                  int i = e.readInt();
                  transpose.chromatic = i;
                  transpose.diatonic = chromatic2diatonic(i);
                  }
            else if (tag == "transposeChromatic")
                  transpose.chromatic = e.readInt();
            else if (tag == "transposeDiatonic")
                  transpose.diatonic = e.readInt();
            else if (tag == "drumset")
                  useDrumset = e.readInt();
            else if (tag == "Drum") {
                  // if we see one of this tags, a custom drumset will
                  // be created
                  if (drumset == 0) {
                        drumset = new Drumset(*smDrumset);
                        drumset->clear();
                        }
                  drumset->load(e);
                  }
            else if (tag == "MidiAction") {
                  NamedEventList a;
                  a.read(e);
                  midiActions.append(a);
                  }
            else if (tag == "Channel" || tag == "channel") {
                  Channel a;
                  a.read(e);
                  channel.append(a);
                  }
            else if (tag == "Articulation") {
                  MidiArticulation a;
                  a.read(e);
                  int n = articulation.size();
                  int i;
                  for(i = 0; i < n; ++i) {
                        if (articulation[i].name == a.name) {
                              articulation[i] = a;
                              break;
                              }
                        }
                  if (i == n)
                        articulation.append(a);
                  }
            else if (tag == "stafftype") {
                  QString val(e.readElementText());
                  if (val == "tablature")
                        useTablature = true;
                  else {
                        qDebug("unknown stafftype <%s>\n", qPrintable(val));
                        e.unknown();
                        }
                  }
            else if (tag == "init") {
                  QString val(e.readElementText());
                  InstrumentTemplate* ttt = searchTemplate(val);
                  if (ttt)
                        init(*ttt);
                  else
                        qDebug("InstrumentTemplate:: init instrument <%s> not found", qPrintable(val));
                  }
            else
                  e.unknown();
            }
      //
      // check bar line spans
      //
      int barLine = 0;
      for (int i = 0; i < staves; ++i) {
            int bls = barlineSpan[i];
            if (barLine) {
                  if (bls)
                        barlineSpan[i] = 0;
                  }
            else {
                  if (bls == 0) {
                        bls = 1;
                        barlineSpan[i] = 1;
                        }
                  barLine = bls;
                  }
            --barLine;
            }
      for (int i = 0; i < MAX_STAVES; ++i) {
            if (clefIdx[i] == CLEF_INVALID)
                  clefIdx[i] = CLEF_G;
            if (staffLines[i] == -1)
                  staffLines[i] = 5;
            }
      if (channel.isEmpty()) {
            Channel a;
            a.chorus       = 0;
            a.reverb       = 0;
            a.name         = "normal";
            a.program      = 0;
            a.bank         = 0;
            a.volume       = 100;
            a.pan         = 60;
            channel.append(a);
            }
      if (useDrumset) {
            if (channel[0].bank == 0)
                  channel[0].bank = 128;
            channel[0].updateInitList();
            }
      if (trackName.isEmpty() && !longNames.isEmpty())
            trackName = longNames[0].name;
      if (id.isEmpty())
            id = trackName.toLower().replace(" ", "-");

      if (staves == 0)
            printf(" 2Instrument: staves == 0 <%s>\n", qPrintable(id));
      }


//---------------------------------------------------------
//   setPitchRange
//---------------------------------------------------------

void InstrumentTemplate::setPitchRange(const QString& s, char* a, char* b) const
      {
      QStringList sl = s.split("-");
      if (sl.size() != 2) {
            *a = 0;
            *b = 127;
            return;
            }
      *a = sl[0].toInt();
      *b = sl[1].toInt();
      }

//---------------------------------------------------------
//   saveInstrumentTemplates
//---------------------------------------------------------

bool saveInstrumentTemplates(const QString& instrTemplates)
      {
      QFile qf(instrTemplates);
      if (!qf.open(QIODevice::WriteOnly)) {
            qDebug("cannot save instrument templates at <%s>\n", qPrintable(instrTemplates));
            return false;
            }
      Xml xml(&qf);
      xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
      xml.stag("museScore");

      foreach(const MidiArticulation& a, articulation)
            a.write(xml);
      xml << "\n";
      foreach(InstrumentGroup* group, instrumentGroups) {
            xml.stag(QString("InstrumentGroup id=\"%1\"").arg(group->id));
            xml.tag("name", group->name);
            if (group->extended)
                  xml.tag("extended", group->extended);
            foreach(InstrumentTemplate* it, group->instrumentTemplates) {
                  it->write(xml);
                  xml << "\n";
                  }
            xml.etag();
            xml << "\n";
            }
      xml.etag();
      qf.close();
      return true;
      }

//---------------------------------------------------------
//   saveInstrumentTemplates1
//---------------------------------------------------------

bool saveInstrumentTemplates1(const QString& instrTemplates)
      {
      QFile qf(instrTemplates);
      if (!qf.open(QIODevice::WriteOnly)) {
            qDebug("cannot save instrument templates at <%s>\n", qPrintable(instrTemplates));
            return false;
            }
      Xml xml(&qf);
      xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
      xml.stag("museScore");

      foreach(InstrumentGroup* group, instrumentGroups) {
            xml.stag(QString("InstrumentGroup id=\"%1\"").arg(group->id));
            xml.tag("name", group->name);
            foreach(InstrumentTemplate* it, group->instrumentTemplates) {
                  it->write1(xml);
                  xml << "\n";
                  }
            xml.etag();
            xml << "\n";
            }
      xml.etag();
      qf.close();
      return true;
      }

//---------------------------------------------------------
//   loadInstrumentTemplates
//---------------------------------------------------------

bool loadInstrumentTemplates(const QString& instrTemplates)
      {
      QFile qf(instrTemplates);
      if (!qf.open(QIODevice::ReadOnly)) {
            qDebug("cannot load instrument templates at <%s>\n", qPrintable(instrTemplates));
            return false;
            }

      XmlReader e(&qf);
      while (e.readNextStartElement()) {
            if (e.name() == "museScore") {
                  while (e.readNextStartElement()) {
                        const QStringRef& tag(e.name());
                        if (tag == "instrument-group" || tag == "InstrumentGroup") {
                              QString id(e.attribute("id"));
                              InstrumentGroup* group = searchInstrumentGroup(id);
                              if (group == 0) {
                                    group = new InstrumentGroup;
                                    instrumentGroups.append(group);
                                    }
                              group->read(e);
                              }
                        else if (tag == "Articulation") {
                              // read global articulation
                              MidiArticulation a;
                              a.read(e);
                              articulation.append(a);
                              }
                        else
                              e.unknown();
                        }
                  }
            }
      // saveInstrumentTemplates1("/home/ws/mops.xml");
      return true;
      }

//---------------------------------------------------------
//   searchTemplate
//---------------------------------------------------------

InstrumentTemplate* searchTemplate(const QString& name)
      {
      foreach(InstrumentGroup* g, instrumentGroups) {
            foreach(InstrumentTemplate* it, g->instrumentTemplates) {
                  if (it->id == name)
                        return it;
                  }
            }
      return 0;
      }

