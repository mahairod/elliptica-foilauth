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
		QCoreApplication::translate("main", "Authentication identity"),
		QCoreApplication::translate("main", "id label") + ", " +
		QCoreApplication::translate("main", "required")
	);
	parser.addOption(accountOption);

	QCommandLineOption passwordOption({"p", "password"},
		QCoreApplication::translate("main", "Global OTP password"),
		QCoreApplication::translate("main", "password")
	);
	parser.addOption(passwordOption);

	QCommandLineOption listOption({"l", "list"},
		QCoreApplication::translate("main", "Lists identities")
	);
	parser.addOption(listOption);

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

	bool list = argsParser.isSet("list");
	QString ident = argsParser.value("ident");
	if (ident.isEmpty() && !list) {
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
				qApp->exit(0);
			} else {
				qInfo() << "Waiting for consuming...";
				QTimer::singleShot(1000, [](){ qApp->exit(4); });
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

	QTextStream out(stdout);
	QObject::connect(&model, &FoilAuthModel::rowsInserted, [&model, &ident, &consumedTokens, &out, passRole, labelRole, list](const QModelIndex &parent, int first, int last)
	{
		for (int ind = first; ind <= last; ++ind) {
			QModelIndex modInd = model.index(ind, 0);
			QString id = model.data(modInd, labelRole).toString();
			qInfo() << "New id: " << id;
			consumedTokens++;
			if (list) {
				out << "ID=" << id << "\n";
				continue;
			}
			if (ident == id) {
				qInfo() << "Found identity " << ident;
				QVariant varVal = model.data(modInd, passRole);
				out << "password:" << varVal.toString();
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
