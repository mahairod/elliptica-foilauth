#ifndef DEBUG
#define QT_NO_DEBUG_OUTPUT
#define QT_NO_INFO_OUTPUT
#endif

#include <stdlib.h>
#include <QList>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>

#include <QtDebug>
#include <gutil_log.h>

#include <QTextStream>

#include <unistd.h>

#include "FoilAuthDefs.h"
#include "FoilAuth.h"
#include "FoilAuthModel.h"


constexpr const char* PASSWD_KEY = "SECRET";

void setupCommandLine(QCommandLineParser& parser) {
	parser.setApplicationDescription("FoilAuth command line OTP generator");
	parser.addHelpOption();
	parser.addVersionOption();

	QCommandLineOption accountOption({"i", "ident", "label"},
		QCoreApplication::tr("main", "Authentication identity"),
		QCoreApplication::tr("main", "id label") + ", " +
		QCoreApplication::tr("main", "required")
	);
	parser.addOption(accountOption);

	QCommandLineOption passwordOption({"p", "password"},
		QCoreApplication::tr("main", "Global OTP password"),
		QCoreApplication::tr("main", "password")
	);
	parser.addOption(passwordOption);

}

int main(int argc, char *argv[]) {
    gutil_log_timestamp = FALSE;
    gutil_log_func = &gutil_log_stderr;
    gutil_log_default.name = FOILAUTH_APP_NAME;
#ifdef DEBUG
    gutil_log_default.level = GLOG_LEVEL_VERBOSE;
#else
    gutil_log_default.level = GLOG_LEVEL_ERR;
#endif

	QCoreApplication app(argc, argv);

	QCommandLineParser argsParser;
	setupCommandLine(argsParser);
	argsParser.process(app);

	char* keyPwd = getenv(PASSWD_KEY);
	QString pwd = keyPwd ? keyPwd : argsParser.value("p");

	QString ident = argsParser.value("ident");
	if (ident.isEmpty()) {
		qCritical() << "Identity not set";
		return 2;
	}

	FoilAuthModel model;

	int consumedTokens = 0;

	QObject::connect(&model, &FoilAuthModel::foilStateChanged, [&model, &consumedTokens]()
	{
		if (model.foilState() == FoilAuthModel::FoilModelReady) {
			qInfo() << "Secrets are ready";
			if (consumedTokens >= model.rowCount()) {
				qInfo() << "Processed " << consumedTokens << "tokens. Exiting";
				exit(0);
			} else {
				qInfo() << "Waiting for consuming...";
				QTimer::singleShot(1000, [](){ exit(4); });
			}
		}
	});

	int passRole = -1;
	int labelRole = -1;
	for (auto it = model.roleNames().cbegin(); it != model.roleNames().cend(); ++it ) {
		if (it.value() == "currentPassword") {
			passRole = it.key();
		}
		if (it.value() == "label") {
			labelRole = it.key();
		}
	}

	QObject::connect(&model, &FoilAuthModel::rowsInserted, [&model, &ident, &consumedTokens, passRole, labelRole](const QModelIndex &parent, int first, int last)
	{
		for (int ind = first; ind <= last; ++ind) {
			QModelIndex modInd = model.index(ind, 0);
			QString id = model.data(modInd, labelRole).toString();
			qInfo() << "New id: " << id;
			consumedTokens++;
			if (ident == id) {
				qInfo() << "Found identity " << ident;
				QVariant varVal = model.data(modInd, passRole);
				QTextStream(stdout) << "password:" << varVal.toString();
				break;
			}
		}
	});

	if (model.unlock(pwd)) {
		return app.exec();
	}

	qDebug() << "Unable to open secrets";
	return 1;

}
