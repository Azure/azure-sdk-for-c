#include "log.h"
#include "logmanager.h"
#include "parse.h"
#include "preferences.h"
#include "PmlElements.h"
#include "OpmlParser.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <QDialog>
#include <QObject>
#include <QCryptographicHash>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonObject>
#include <QMetaObject>
#include <QPolygonF>
#include <QColor>
#include <QPointF>
#include <QRectF>
#include <QFont>
#include <QList>
#include <QByteArray>
#include <QString>
#include <thread>
#include <mutex>
#include <condition_variable>

// Log implementation
using namespace pml;

const std::string LogStream::STR_LEVEL[6] = {"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};

const char* log::GetVersion()    {   return pml::log::VERSION_STRING; }
const char* log::GetGitDate()    {   return pml::log::GIT_DATE;  }
const char* log::GetGitTag()     {   return pml::log::GIT_TAG;   }
const char* log::GetGitBranch()  {   return pml::log::GIT_BRANCH; }

LogStream pmlLog(enumLevel elevel, const std::string& sPrefix)
{
    return LogStream(elevel, sPrefix);
}

LogManager& LogManager::Get()
{
    static LogManager lm;
    return lm;
}

LogManager::LogManager() : m_nOutputIdGenerator(0),
m_pThread(std::make_unique<std::thread>([this]{Loop();}))
{}

LogManager::~LogManager()
{
    Stop();
}

void LogManager::Stop()
{
    if(m_pThread)
    {
        m_bRun = false;
        m_cv.notify_one();
        m_pThread->join();
        m_pThread = nullptr;
    }
}

void LogManager::Flush(const std::stringstream& ssLog, enumLevel eLevel, const std::string& sPrefix)
{
    m_qLog.enqueue(logEntry(ssLog.str(), eLevel,sPrefix));
    m_cv.notify_one();
}

void LogManager::Loop()
{
    while(m_bRun)
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        logEntry entry;
        while(m_qLog.try_dequeue(entry))
        {
            for(auto& pairOutput : m_mOutput)
            {
                pairOutput.second->Flush(entry.eLevel, entry.sLog, entry.sPrefix);
            }
        }
        m_cv.wait(lk);
    }
}

void LogManager::SetOutputLevel(size_t nIndex, enumLevel eLevel)
{
    std::lock_guard<std::mutex> lg(m_mutex);
    auto itOutput = m_mOutput.find(nIndex);
    if(itOutput != m_mOutput.end())
    {
        itOutput->second->SetOutputLevel(eLevel);
    }
}

void LogManager::SetOutputLevel(enumLevel eLevel)
{
    std::lock_guard<std::mutex> lg(m_mutex);
    for(auto& pairOutput : m_mOutput)
    {
        pairOutput.second->SetOutputLevel(eLevel);
    }
}

size_t LogManager::AddOutput(std::unique_ptr<LogOutput> pLogout)
{
    std::lock_guard<std::mutex> lg(m_mutex);
    ++m_nOutputIdGenerator;
    return m_mOutput.insert(std::make_pair(m_nOutputIdGenerator, move(pLogout))).first->first;
}

void LogManager::RemoveOutput(size_t nIndex)
{
    std::lock_guard<std::mutex> lg(m_mutex);
    m_mOutput.erase(nIndex);
}

LogStream::LogStream(enumLevel eLevel, const std::string& sPrefix) : m_logLevel(eLevel), m_sPrefix(sPrefix)
{}

LogStream::~LogStream()
{
    m_stream << "\n";
    flush();
}

LogStream::LogStream(const LogStream& lg) : m_logLevel(lg.GetLevel()), m_sPrefix(lg.GetPrefix())
{
    m_stream << lg.GetStream().rdbuf();
}

LogStream& LogStream::operator=(const LogStream& lg)
{
    if(&lg != this)
    {
        m_stream.str(std::string());
        m_stream.clear();

        m_stream << lg.GetStream().rdbuf();
        m_logLevel = lg.GetLevel();
        m_sPrefix = lg.GetPrefix();
    }
    return *this;
}

void LogStream::flush()
{
    LogManager::Get().Flush(m_stream, m_logLevel, m_sPrefix);

    m_stream.str(std::string());
    m_stream.clear();
}

void LogStream::SetOutputLevel(size_t nIndex, enumLevel eLevel)
{
    LogManager::Get().SetOutputLevel(nIndex, eLevel);
}

void LogStream::SetOutputLevel(enumLevel eLevel)
{
    LogManager::Get().SetOutputLevel(eLevel);
}

size_t LogStream::AddOutput(std::unique_ptr<LogOutput> pLogout)
{
    return LogManager::Get().AddOutput(std::move(pLogout));
}

void LogStream::RemoveOutput(size_t nIndex)
{
    LogManager::Get().RemoveOutput(nIndex);
}

LogStream& LogStream::operator<<(ManipFn manip)
{
    manip(m_stream);

    if (manip == static_cast<ManipFn>(std::flush)
     || manip == static_cast<ManipFn>(std::endl ) )
        this->flush();

    return *this;
}

LogStream& LogStream::operator<<(FlagsFn manip)
{
    manip(m_stream);
    return *this;
}

LogStream& LogStream::operator()(enumLevel e)
{
    m_logLevel = e;
    return *this;
}

LogStream& LogStream::SetLevel(enumLevel e)
{
    m_logLevel = e;
    return *this;
}

void LogStream::Stop()
{
    LogManager::Get().Stop();
}

void LogOutput::Flush(enumLevel eLogLevel, const std::string& sLog, const std::string& sPrefix)
{
    if(eLogLevel >= m_eLevel)
    {
        std::cout << Timestamp().str();
        std::cout << LogStream::STR_LEVEL[eLogLevel] << "\t" << "[" << sPrefix << "]\t" << sLog;
    }
}

std::stringstream LogOutput::Timestamp()
{
    std::stringstream ssTime;
    if(m_nTimestamp != TS_NONE)
    {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        if((m_nTimestamp & TS_DATE))
        {
            ssTime << std::put_time(localtime(&in_time_t), "%Y-%m-%d ");
        }
        if((m_nTimestamp & TS_TIME))
        {
             ssTime << std::put_time(localtime(&in_time_t), "%H:%M:%S");
        }
        switch(m_eResolution)
        {
            case TSR_MILLISECOND:
                ssTime << "." << std::setw(3) << std::setfill('0') << (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()%1000);
                break;
            case TSR_MICROSECOND:
                ssTime << "." << std::setw(6) << std::setfill('0') << (std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count()%1000000);
                break;
            case TSR_NANOSECOND:
                ssTime << "." << std::setw(9) << std::setfill('0') << (std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count()%1000000000);
                break;
            default:
                break;
        }
        ssTime << "\t";
    }
    return ssTime;
}

void LogOutput::SetOutputLevel(enumLevel eLevel)
{
    m_eLevel = eLevel;
}

enumLevel LogOutput::GetOutputLevel() const
{
    return m_eLevel;
}

// Parse implementation
Parse::Parse() {}

void Parse::signup(QString username, QString password, QString email) {}
void Parse::deleteUser(QString objectId) {}
void Parse::login(QString username, QString password) {}
void Parse::logout() {}
void Parse::passwordReset(QString email) {}
void Parse::create(QString doc) {}
QNetworkReply* Parse::get(QString include) { return nullptr; }
void Parse::update(QString doc) {}
void Parse::deleteObject(QString doc) {}
bool Parse::ensureEndPointHasPrefix(QString prefix) { return false; }
void Parse::pmlList() {}
void Parse::uploadPML(QString type) {}
void Parse::postPML(QString type, QString title, QString description, QByteArray pml_file, QString keywords) {}
void Parse::updatePMLfile(QString objectId, QString pml_file) {}
void Parse::saveCurrentSession(QString title, QString description, QString pml_file) {}
QNetworkReply* Parse::uploadFile(QUrl url, QString name) { return nullptr; }
QNetworkReply* Parse::uploadFileData(QByteArray data, QString name) { return nullptr; }
QNetworkReply* Parse::deleteFile(QString fileName) { return nullptr; }
QNetworkReply* Parse::query(QString endPoint, QUrlQuery extraParams) { return nullptr; }
QString Parse::getApplicationId() const { return applicationId; }
void Parse::setApplicationId(const QString& res) { applicationId = res; }
QString Parse::getSessionId() const { return sessionId; }
void Parse::setSessionId(const QString& res) { sessionId = res; }
QString Parse::getUserId() const { return userId; }
void Parse::setUserId(const QString& res) { userId = res; }
QString Parse::getMasterKey() const { return masterKey; }
void Parse::setMasterKey(const QString& res) { masterKey = res; }
bool Parse::isReady() const { return true; }
void Parse::postProcessGet(QJsonObject obj) {}
QString Parse::generatePmlXml(QJsonObject json) { return QString(); }
void Parse::processPML(QString type, QString pmlFileName) {}
void Parse::updatePML(QString doc) {}
void Parse::updateSnapfile(QString objectId, QString pml_file, QJsonObject obj) {}

// Preferences Implementation
Preferences::Preferences(QWidget *parent, Settings *settings) : QDialog(parent), settings(settings), processing(false) {}
Preferences::~Preferences() {}

void Preferences::closeEvent(QCloseEvent *event) {}

void Preferences::on_Panel_currentRowChanged(int currentRow) {}
void Preferences::on_cellsX_valueChanged(int value) {}
void Preferences::on_cellsY_valueChanged(int value) {}
void Preferences::on_sizeX_valueChanged(double value) {}
void Preferences::on_sizeY_valueChanged(double value) {}
void Preferences::on_courant_valueChanged(double value) {}
void Preferences::on_time_valueChanged(double value) {}
void Preferences::on_timesteps_valueChanged(int value) {}
void Preferences::on_PMLlayers_valueChanged(int value) {}
void Preferences::on_sigmaxMax_valueChanged(double value) {}
void Preferences::on_sigmayMax_valueChanged(double value) {}
void Preferences::on_Auto_clicked() {}
void Preferences::on_NoOfThreads_valueChanged(int value) {}
void Preferences::on_m_valueChanged(double value) {}
void Preferences::on_sampleDistance_valueChanged(int value) {}
void Preferences::on_width_valueChanged(int value) {}
void Preferences::on_height_valueChanged(int value) {}
void Preferences::on_DrawNthField_valueChanged(int value) {}

// PmlElements implementation
PmlBasicsProperties::PmlBasicsProperties() : _elevation(0), _lineWidth(0) {}
PmlBasicsProperties::PmlBasicsProperties(const PmlBasicsProperties &properties) : _lineType(properties._lineType), _color(properties._color), _elevation(properties._elevation), _lineWidth(properties._lineWidth) {}
PmlBasicsProperties& PmlBasicsProperties::operator=(PmlBasicsProperties &properties)
{
    _lineType = properties._lineType;
    _color = properties._color;
    _elevation = properties._elevation;
    _lineWidth = properties._lineWidth;
    return *this;
}
void PmlBasicsProperties::setLineType(QString lineType) { _lineType = lineType; }
void PmlBasicsProperties::setColor(QColor color) { _color = color; }
void PmlBasicsProperties::setElevation(qreal elevation) { _elevation = elevation; }
void PmlBasicsProperties::setLineWidth(int lineWidth) { _lineWidth = lineWidth; }
QString PmlBasicsProperties::lineType() { return _lineType; }
QColor PmlBasicsProperties::color() { return _color; }
qreal PmlBasicsProperties::elevation() { return _elevation; }
int PmlBasicsProperties::lineWidth() { return _lineWidth; }

PmlEllipse::PmlEllipse() {}
PmlEllipse::PmlEllipse(const PmlEllipse &ellipse) : PmlBasicsProperties(ellipse), _position(ellipse._position), _size(ellipse._size) {}
PmlEllipse& PmlEllipse::operator=(PmlEllipse& ellipse)
{
    PmlBasicsProperties *bp1, *bp2;
    bp1 = this;
    bp2 = &ellipse;
    *bp1 = *bp2;
    _position = ellipse._position;
    _size = ellipse._size;
    return *this;
}
void PmlEllipse::setPosition(QPointF position) { _position = position; }
void PmlEllipse::setX(qreal x) { _position.setX(x); }
void PmlEllipse::setY(qreal y) { _position.setY(y); }
void PmlEllipse::setSize(QSizeF size) { _size = size; }
QPointF PmlEllipse::position() { return _position; }
QSizeF PmlEllipse::size() { return _size; }

PmlHatch::PmlHatch() {}
PmlHatch::PmlHatch(const PmlHatch& hatch) : PmlBasicsProperties(hatch)
{
    foreach(PmlPolygon polygon, hatch._polygons)
    {
        _polygons.append(polygon);
    }
}
PmlHatch& PmlHatch::operator=(PmlHatch& hatch)
{
    foreach(PmlPolygon polygon, hatch._polygons)
    {
        _polygons.append(polygon);
    }
    PmlBasicsProperties *pp1, *pp2;
    pp1 = this;
    pp2 = &hatch;
    *pp1 = *pp2;
    return *this;
}
void PmlHatch::addPolygon(PmlPolygon polygon) { _polygons.append(polygon); }
void PmlHatch::clearPolygons() { _polygons.clear(); }
bool PmlHatch::isPolygonsEmpty() { return _polygons.isEmpty(); }
QList<PmlPolygon> PmlHatch::polygons() { return _polygons; }

PmlPolyline::PmlPolyline() : _closed(0) {}
PmlPolyline::PmlPolyline(const PmlPolyline &polyline) : QPolygonF(polyline), PmlBasicsProperties(polyline), _closed(polyline._closed) {}
PmlPolyline& PmlPolyline::operator=(PmlPolyline &polyline)
{
    _closed = polyline._closed;
    QPolygonF *pf1, *pf2;
    pf1 = this;
    pf2 = &polyline;
    *pf1 = *pf2;
    PmlBasicsProperties *bp1, *bp2;
    bp1 = this;
    bp2 = &polyline;
    *bp1 = *bp2;
    return *this;
}
void PmlPolyline::setClosed(bool closed) { _closed = closed; }
bool PmlPolyline::isClosed() { return _closed; }

PmlText::PmlText() : _rotation(0), _elevation(0) {}
PmlText::PmlText(const PmlText &text) : _text(text._text), _position(text._position), _rotation(text._rotation), _font(text._font), _color(text._color), _elevation(text._elevation) {}
PmlText& PmlText::operator=(PmlText& text)
{
    _text = text._text;
    _position = text._position;
    _rotation = text._rotation;
    _font = text._font;
    _color = text._color;
    _elevation = text._elevation;
    return *this;
}
void PmlText::setText(QString text) { _text = text; }
void PmlText::setPosition(QPointF position) { _position = position; }
void PmlText::setRotation(qreal rotation) { _rotation = rotation; }
void PmlText::setFont(QFont font) { _font = font; }
void PmlText::setColor(QColor color) { _color = color; }
void PmlText::setElevation(qreal elevation) { _elevation = elevation; }
QString PmlText::text() { return _text; }
QPointF PmlText::position() { return _position; }
qreal PmlText::rotation() { return _rotation; }
QFont PmlText::font() { return _font; }
QColor PmlText::color() { return _color; }
qreal PmlText::elevation() { return _elevation; }

// OpmlParser implementation
COpmlParser::COpmlParser(CFeedEngine& aFeedEngine, RFs& aFs) : iFeedEngine(aFeedEngine), iFs(aFs) {}
COpmlParser::~COpmlParser() {}

void COpmlParser::ParseOpmlL(const TFileName &feedFileName, TBool aSearching) {}
void COpmlParser::OnStartDocumentL(const Xml::RDocumentParameters& aDocParam, TInt aErrorCode) {}
void COpmlParser::OnEndDocumentL(TInt aErrorCode) {}
void COpmlParser::OnStartElementL(const Xml::RTagInfo& aElement, const Xml::RAttributeArray& aAttributes, TInt aErrorCode) {}
void COpmlParser::OnEndElementL(const Xml::RTagInfo& aElement, TInt aErrorCode) {}
void COpmlParser::OnContentL(const TDesC8& aBytes, TInt aErrorCode) {}
void COpmlParser::OnStartPrefixMappingL(const RString& aPrefix, const RString& aUri, TInt aErrorCode) {}
void COpmlParser::OnEndPrefixMappingL(const RString& aPrefix, TInt aErrorCode) {}
void COpmlParser::OnIgnorableWhiteSpaceL(const TDesC8& aBytes, TInt aErrorCode) {}
void COpmlParser::OnSkippedEntityL(const RString& aName, TInt aErrorCode) {}
void COpmlParser::OnProcessingInstructionL(const TDesC8& aTarget, const TDesC8& aData, T
