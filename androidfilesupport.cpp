#include <QtAndroid>
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#include <QAndroidActivityResultReceiver>

#include "core/emu.h"

// A handler to open android content:// URLs.
// Based on code by Florin9doi: https://github.com/nspire-emus/firebird/pull/94/files
FILE *gui_open(const char *path, const char *mode)
{
    const char pattern[] = "content:";
    if(strncmp(pattern, path, sizeof(pattern)-1) != 0)
        return fopen(path, mode);

    QString android_mode; // Why did they have to NIH...
    if(strcmp(mode, "rb") == 0)
        android_mode = QStringLiteral("r");
    else if(strcmp(mode, "r+b") == 0)
        android_mode = QStringLiteral("rw");
    else if(strcmp(mode, "wb") == 0)
        android_mode = QStringLiteral("rwt");
    else
        return nullptr;

    QAndroidJniObject jpath = QAndroidJniObject::fromString(QString::fromUtf8(path));
    QAndroidJniObject jmode = QAndroidJniObject::fromString(android_mode);
    QAndroidJniObject uri = QAndroidJniObject::callStaticObjectMethod(
                "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;",
                jpath.object<jstring>());

    QAndroidJniObject contentResolver = QtAndroid::androidActivity()
            .callObjectMethod("getContentResolver",
                              "()Landroid/content/ContentResolver;");

    QAndroidJniObject parcelFileDescriptor = contentResolver
            .callObjectMethod("openFileDescriptor",
                              "(Landroid/net/Uri;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;",
                              uri.object<jobject>(), jmode.object<jobject>());

    QAndroidJniEnvironment env;
    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    // The file descriptor needs to be duplicated as
    QAndroidJniObject parcelFileDescriptorDup = parcelFileDescriptor
            .callObjectMethod("dup",
                              "()Landroid/os/ParcelFileDescriptor;");

    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    int fd = parcelFileDescriptorDup.callMethod<jint>("detachFd", "()I");
    if(fd < 0)
        return nullptr;

    return fdopen(fd, mode);
}
