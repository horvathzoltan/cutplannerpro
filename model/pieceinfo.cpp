// #include "pieceinfo.h"

// PieceInfo::PieceInfo()
//     : m_length_mm(0), m_isCompleted(false)
// {}

// PieceInfo::PieceInfo(int length_mm, const QString& owner, const QString& reference, bool completed)
//     : m_length_mm(length_mm),
//     m_ownerName(owner),
//     m_externalReference(reference),
//     m_isCompleted(completed)
// {}

// int PieceInfo::length_mm() const {
//     return m_length_mm;
// }

// void PieceInfo::setLength_mm(int value) {
//     m_length_mm = value;
// }

// QString PieceInfo::ownerName() const {
//     return m_ownerName;
// }

// void PieceInfo::setOwnerName(const QString& value) {
//     m_ownerName = value;
// }

// QString PieceInfo::externalReference() const {
//     return m_externalReference;
// }

// void PieceInfo::setExternalReference(const QString& value) {
//     m_externalReference = value;
// }

// bool PieceInfo::isCompleted() const {
//     return m_isCompleted;
// }

// void PieceInfo::setCompleted(bool value) {
//     m_isCompleted = value;
// }

// bool PieceInfo::isValid() const {
//     return m_length_mm > 0 && !m_ownerName.isEmpty();
// }

// QString PieceInfo::displayText() const {
//     return QString("%1 • %2 • %3 mm")
//         .arg(m_ownerName)
//         .arg(m_externalReference)
//         .arg(m_length_mm);
// }
