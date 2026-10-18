#include "worksheetsummary_builder.h"

#include "common/filenamehelper.h"

#include <QFile>
#include <QCryptographicHash>

#include <model/repositories/cuttingrequestrepository.h>

QByteArray
WorkSheetSummaryBuilder::calculateFileHash(
    const QString& fileName)
{
    QString filePath =
        FileNameHelper::instance()
            .getCuttingPlanFilePath(
                fileName);

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }

    QByteArray content =
        file.readAll();

    return
        QCryptographicHash::hash(
            content,
            QCryptographicHash::Sha256);
}

std::optional<WorkSheetSummary>
WorkSheetSummaryBuilder::build(
    const QString& fileName)
{
    WorkSheetSummary summary;

    summary.fileName =
        fileName;

    summary.filePath =
        FileNameHelper::instance()
            .getCuttingPlanFilePath(
                fileName);

    if (!FileHelper::isExists(summary.filePath))
    {
        return std::nullopt;
    }

    summary.fileHash =
        calculateFileHash(
            fileName);

    summary.presence =
        WorkSheetPresence::Present;

    auto requests =
        CuttingRequestRepository::loadRequests(
            summary.filePath);

    //TODO üres worksheet != hibás worksheet

    summary.requestCount =
        requests.size();

    for (const auto& req : requests)
    {
        switch (req.state)
        {
        case Cutting::Plan::RequestState::Cut:
            ++summary.completedCount;
            break;

        case Cutting::Plan::RequestState::NotCut:
            ++summary.uncompletedCount;
            break;

        case Cutting::Plan::RequestState::Unknown:
            ++summary.undefinedCount;
            break;
        }
    }

    return summary;
}
