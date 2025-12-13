#include "utils.h"

QString hashPassword(QString passwd) {
    QByteArray hashValue = QCryptographicHash::hash(passwd.toUtf8(), QCryptographicHash::Sha512) ;
    // qDebug() << "Password: " << passwd << " Hash(SHA-512): " << hashValue.toBase64() ;
    return hashValue.toBase64() ;
}
