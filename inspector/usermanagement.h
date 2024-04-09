#ifndef USERSMANAGMENT_H
#define USERSMANAGMENT_H

#include <QObject>
#include <QFile>
#include <QUuid>
#include <QSettings>

#include "inspectordefines.h"
#include "interfaces.h"


typedef struct {
	QString uuid;
	QString name;
	int accessLevel;
	uint passwordHash;
	QString cardId;
} USER;


class UserManagement : public QObject
{
    Q_OBJECT

public:
	static UserManagement& instance()
	{
		static UserManagement instance;
		return instance;
	}

	enum UserProperty {accessLevel, password, cardId};
	enum UserImportOption {append, replace};

	void addUser(const USER user);
	bool containsProperty(const UserProperty userProperty, const QVariant property) const;
	void deleteUser(const QString uuid);
	void editUser(const QString uuid, const UserProperty userProperty, const QVariant property);
	void exportUsers(const QString filePath);
	int getBaseAccessLevel() const { return _baseAccessLevel; }
	USER getUserByProperty(const UserProperty userProperty, const QVariant property) const;
	void importUsers(const UserImportOption option, const QString filePath);
	QList<USER> users() const;

private:
	UserManagement(QObject* parent = nullptr);
	~UserManagement() = default;
	UserManagement(const UserManagement&) = delete;
	UserManagement& operator=(const UserManagement&) = delete;

	QHash<QString, USER> loadOldUsers();
	QHash<QString, USER> loadUsers();
	void saveUsers(const QList<USER> users);
	void setBaseAccessLevel();

	int _baseAccessLevel = kAccessLevelGuest;
	static USER _defaultUser;
	QHash<QString, USER> _users;
};
#endif // USERSMANAGMENT_H
