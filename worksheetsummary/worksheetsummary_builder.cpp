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

QVector<WorkSheetOrderInfo>
WorkSheetSummaryBuilder::buildOrderInfos(
    const QVector<Cutting::Plan::Request>& requests)
{
    QMap<QString, WorkSheetOrderInfo> orders;

    for (const auto& req : requests)
    {
        auto& order =
            orders[req.externalReference];

        if (order.externalReference.isEmpty())
        {
            order.externalReference =
                req.externalReference;

            order.ownerName =
                req.ownerName;

            order.dueDate =
                req.dueDate;

            order.productTypeId =
                req.productTypeId;

            order.productSubtypeId =
                req.productSubtypeId;
        }

        ++order.requestCount;

        switch (req.state)
        {
        case Cutting::Plan::RequestState::Cut:
            ++order.completedCount;
            break;

        case Cutting::Plan::RequestState::NotCut:
            ++order.uncompletedCount;
            break;

        case Cutting::Plan::RequestState::Unknown:
            ++order.undefinedCount;
            break;
        }
    }

    return orders.values().toVector();
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

    summary.orders =
        buildOrderInfos(
            requests);

    return summary;
}
