#include "usermanagement.h"

#include <QDebug>

#include <audittrail.h>


USER UserManagement::_defaultUser = { "","Bertram", kAccessLevelBertram, qHash(QString("9955")), "" };

UserManagement::UserManagement(QObject* parent) : QObject(parent)
{
	_users = loadUsers();
    
	saveUsers(_users.values());

	setBaseAccessLevel();
}

void UserManagement::addUser(const USER user)
{
    _users.insert(user.uuid, user);
	saveUsers(_users.values());

    AuditTrail::message(tr("Added user %1 with access level %2").arg(user.name).arg(user.accessLevel));
    
	setBaseAccessLevel();
}

bool UserManagement::containsProperty(const UserProperty userProperty, const QVariant property) const
{
	if (!property.isNull()) {

		switch (userProperty) {
		case password:
			if (_defaultUser.passwordHash == property.toUInt()) {
				return true;
			}
			for (USER user : _users) {
				if (user.passwordHash == property.toUInt()) {
					return true;
				}
			}
			break;
		case cardId:
			for (USER user : _users) {
				if (user.cardId == property.toString()) {
					return true;
				}
			}
			break;
		case accessLevel:
			if (_defaultUser.accessLevel == property.toInt()) {
				return true;
			}
			for (USER user : _users) {
				if (user.accessLevel == property.toInt()) {
					return true;
				}
			}
			break;
		}
	}

	return false;
}

void UserManagement::deleteUser(const QString uuid)
{
    USER user = _users.value(uuid);
    AuditTrail::message(tr("Deleted user %1").arg(user.name));
    
	_users.remove(uuid);
	saveUsers(_users.values());
    
	setBaseAccessLevel();
}

void UserManagement::editUser(const QString uuid, const UserProperty userProperty, const QVariant property)
{
	USER user = _users.value(uuid);

	switch (userProperty) {
	case accessLevel:
		user.accessLevel = property.toInt();
        AuditTrail::message(tr("Set access level of user %1 to %2").arg(user.name).arg(user.accessLevel));
		break;
	case password:
		user.passwordHash = property.toUInt();
        AuditTrail::message(tr("Changed password of user %1").arg(user.name));
		break;
	case cardId:
		user.cardId = property.toString();
        AuditTrail::message(tr("Set card id of user %1 to %2").arg(user.name).arg(user.cardId));
		break;
	}

	_users.insert(uuid, user);
	saveUsers(_users.values());

	setBaseAccessLevel();
}

void UserManagement::exportUsers(const QString filePath)
{
	if (!filePath.isEmpty()) {
		QFile file(filePath);
		if (file.open(QFile::WriteOnly))
		{
			QList<USER> users = _users.values();
			QTextStream stream(&file);
			for (USER user : users) {
				stream << user.uuid << "," << user.name << "," << user.accessLevel << "," << user.passwordHash << "," << user.cardId << "\n";
			}
			file.close();
            
            AuditTrail::message("User database exported");
		}
	}
}

QList<USER> UserManagement::users() const
{
	return _users.values();
}

USER UserManagement::getUserByProperty(const UserProperty userProperty, const QVariant property) const
{
	if (!property.isNull()) {
		switch (userProperty) {
		case password:
			if (_defaultUser.passwordHash == property.toUInt()) {
				return _defaultUser;
			}
			for (const USER &user : _users) {
				if (user.passwordHash == property.toUInt()) {
					return user;
				}
			}
			break;
		case cardId:
			for (const USER &user : _users) {
				if (user.cardId == property.toString()) {
					return user;
				}
			}
			break;
        default:
            assert(false);
		}
	}
    
	return USER();
}

void UserManagement::importUsers(const UserImportOption option, const QString filePath)
{
	if (!filePath.isEmpty()) {

		QList<USER> users;

		QFile file(filePath);
		if (file.open(QFile::ReadOnly))
		{
			QTextStream stream(&file);

			while (!stream.atEnd()) {
				QString line = stream.readLine();
				QStringList elements = line.split(',');

				if (elements.size() == 5) {

					USER user;
					user.uuid = elements[0];
					user.name = elements[1];
					user.accessLevel = elements[2].toInt();
					user.passwordHash = elements[3].toUInt();
					user.cardId = elements[4];

					users.append(user);
				}
			}

			file.close();
		}

		if (option == append) {
			users.append(_users.values());
		}
		saveUsers(users);

		_users = loadUsers();
	}
}

QHash<QString, USER> UserManagement::loadOldUsers()
{
	QHash<QString, USER> users;
	QSettings settings;

	std::map<int, QString> accessLevel {
		{kAccessLevelUser, tr("User")},
		{kAccessLevelService, tr("Service")},
		{kAccessLevelSysOp, tr("SysOp")},
		{kAccessLevelAdmin, tr("Admin")}
	};

	for (const auto &level : accessLevel) {
		if (settings.contains(kPassword.arg(level.first))) {
			USER user;
			user.uuid = QUuid::createUuid().toString();
			user.name = level.second;
			user.accessLevel = level.first;
			user.passwordHash = settings.value(kPassword.arg(level.first)).toUInt();

			users.insert(user.uuid, user);

			settings.remove(kPassword.arg(level.first));
		}
	}

	return users;
}

QHash<QString, USER> UserManagement::loadUsers()
{
	QHash<QString, USER> users;
	QSettings settings;

	//Benutzer laden
	int size = settings.beginReadArray(kUsers);
	for (int i = 0; i < size; ++i) {
		settings.setArrayIndex(i);
		USER user;
		user.uuid = settings.value("uuid").toString();
		user.name = settings.value("name").toString();
		user.accessLevel = settings.value("accessLevel").toInt();
		user.passwordHash = settings.value("password").toUInt();
		user.cardId = settings.value("cardId").toString();

		users.insert(user.uuid, user);
	}
	settings.endArray();

    // include legacy users (this happens only once)
    auto legacyUsers = loadOldUsers();
    for (auto user : legacyUsers) {
        if (!users.count(user.uuid)) {
            users[user.uuid] = user;
        }
    }
    
	return users;
}

void UserManagement::saveUsers(const QList<USER> users)
{
	QSettings settings;
	settings.remove(kUsers);

	settings.beginWriteArray(kUsers);
	for (int i = 0; i < users.size(); ++i) {
		settings.setArrayIndex(i);
		settings.setValue("uuid", users.at(i).uuid);
		settings.setValue("name", users.at(i).name);
		settings.setValue("accessLevel", users.at(i).accessLevel);
		settings.setValue("password", users.at(i).passwordHash);
		settings.setValue("cardId", users.at(i).cardId);
	}
	settings.endArray();
}

void UserManagement::setBaseAccessLevel()
{
	int accessLevel = kAccessLevelGuest;

	QList<USER> users = _users.values();

	if (!users.isEmpty()) {
		int lvl = kAccessLevelAdmin;

		QList<USER>::const_iterator i;
		for (i = users.constBegin(); i != users.constEnd(); ++i) {
			if (lvl > i->accessLevel) {
				lvl = i->accessLevel;
			}
		}

		accessLevel = std::max(lvl - 1, 0);
	}
	else {
		accessLevel = kAccessLevelAdmin;
	}

	_baseAccessLevel = accessLevel;
}
