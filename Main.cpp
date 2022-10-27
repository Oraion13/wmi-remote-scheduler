#pragma once

#include <iostream>
#include <stdlib.h>
#include <string>
#include <Wbemidl.h>
#include <wbemcli.h>

#include <json/json.h>

#include "Utils.h";
#include "RemoteTasks.h";

using namespace std;

void post_login(Json::Value user_token) {
	RemoteTasks* remoteTasks = new RemoteTasks;

	IWbemServices* pSvc = remoteTasks->wmi_login(user_token["username"].asCString(), user_token["password"].asCString(), user_token["computer_name"].asCString());
	if (pSvc == NULL) {
		cout << "Cannot login" << endl;
		return;
	}

	if (remoteTasks->isWindows(pSvc, user_token)) {
		cout << "logged in successfully!" << endl;
	}
}

int main(int argc, char** argv) {
	cout << "Content-type:application/json\r\n\r\n";
	Utils* utils = new Utils;

	// get the requested method
	char *meth;
	size_t len = 10;
	_dupenv_s(&meth, &len, "REQUEST_METHOD");

	if (strcmp(meth, "GET") == 0) {
		cout << "Get all tasks" << endl;
	}
	else if (strcmp(meth, "POST") == 0) {
		// get the request body
		Json::Value body = utils->readRequestBody();

		if (body != NULL && !body.isNull()) {
			// choose to login or add task
			if (body["username"]) {
				post_login(body);
			}
			else {
				cout << "Post task" << endl;
			}
		}
	}
	else if (strcmp(meth, "DELETE") == 0) {
		cout << "Delete a task" << endl;
	}
}