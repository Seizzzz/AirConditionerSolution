#ifndef TOOLS_H
#define TOOLS_H

#include <QJsonObject>
#include <QJsonDocument>
#include <QWebSocket>

enum class MsgType
{
    TurnOnOff,
    ChangeState,
    ShowCurrentMoney,
    CheckIn,
    CheckOut,
    ShowAllStatus,
    ForceShutDown,
    ReportError,
    ShowRecord
};

enum class ModeType
{
    CoolMode,
    WarmMode,
    OffMode
};

enum class WindType
{
    HighWind,
    MiddleWind,
    LowWind
};

enum class TurnOnOffType
{
    TurnOn,
    TurnOff
};

#define JSONAME_TYPE "MsgType"
#define JSONAME_ROOMID "RoomId"
#define JSONAME_USERID "UserId"
#define JSONAME_WNDSPD "WindSpeed"
#define JSONAME_TEMP "TargetTemp"
#define JSONAME_MODE "Mode"
#define JSONAME_POWER "TurnOnOff"
#define JSONAME_AIRDATA "AirData"
#define JSONAME_ACK "Ack"
#define JSONAME_MONEY "Money"

#define JSONAME_ROOMTEMP "RoomTemp"
#define JSONAME_DEFAULTEMP "DefaultTemp"
#define JSONAME_FEERATE "FeeRate"
#define JSONAME_HIGHTEMP "HighestTemp"
#define JSONAME_LOWTEMP "LowestTemp"
#define JSONAME_HIGHFEE "HighFee"
#define JSONAME_MIDFEE "MidFee"
#define JSONAME_LOWFEE "LowFee"

#define JSONAME_DETAIL "CostDetail"
#define JSONAME_STIME "TimeStart"
#define JSONAME_ETIME "TimeEnd"
#define JSONAME_COST "Cost"
#define JSONAME_AIRSTATUS "AirStatus"
#define JSONAME_ISON "IsOn"
#define JSONAME_ALL "AllRecord"

#define DBNAME_DATABASE "hotel"

#define DBNAME_TABLE_ROOM "room"
#define DBNAME_FIELD_RID "Roomid"
#define DBNAME_FIELD_UID "UserId"
#define DBNAME_FIELD_WNDSPD "WindSpeed"
#define DBNAME_FIELD_TEMP "tempture"
#define DBNAME_FIELD_POWER "AirState"
#define DBNAME_FIELD_TIME "Time"
#define DBNAME_FIELD_MODE "Mode"

#define DBNAME_TABLE_ROOMSTATE "roomstate"
#define DBNAME_FIELD_RID2 "RoomId"
#define DBNAME_FIELD_STATE "State"

#define DB_ADDR "127.0.0.1"
#define DB_PORT 3306

const int INTERVAL_IMMEDIATE = 1;
const int INTERVAL_RECONNECT = 3000;

inline QJsonObject string2jsonobj(const QString& str)
{
    //return QJsonDocument::fromJson(str.toLocal8Bit().data()).object();
    return QJsonDocument::fromJson(str.toUtf8()).object();
}

inline QString jsonobj2string(const QJsonObject& obj)
{
    //return QString(QJsonDocument(obj).toJson());
    return QString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

inline QString getIdentifier(QWebSocket* peer)
{
    return QStringLiteral("%1:%2").arg(peer->peerAddress().toString()).arg(QString::number(peer->peerPort()));
}

#endif // TOOLS_H
