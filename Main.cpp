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

void post_login(Utils* utils, Json::Value user_token) {
	RemoteTasks* remoteTasks = new RemoteTasks;

	IWbemServices* pSvc = remoteTasks->wmi_login(user_token["username"].asCString(), user_token["password"].asCString(), user_token["computer_name"].asCString());
	if (pSvc == NULL) {
		//char  buffer[200];
		//sprintf_s(buffer, 200, "Cannot login");
		//utils->sendStatus(400, 0, buffer);
		return;
	}

	if (remoteTasks->isWindows(pSvc, user_token)) {
		char  buffer[200];
		sprintf_s(buffer, 200, "loggedin successfully!");
		utils->sendStatus(200, 1, buffer);
	}
}

void post_task(Utils* utils, Json::Value tokens) {
	RemoteTasks* remoteTasks = new RemoteTasks;
	Json::Value user_token = tokens[0];
	Json::Value task_token = tokens[1];

	IWbemServices* pSvc = remoteTasks->wmi_login(user_token["username"].asCString(), user_token["password"].asCString(), user_token["computer_name"].asCString());
	if (pSvc == NULL) {
		//char  buffer[200];
		//sprintf_s(buffer, 200, "Cannot login");
		//utils->sendStatus(400, 0, buffer);
		return;
	}

	if (remoteTasks->wmi_post_tasks(pSvc, user_token, task_token)) {
		char  buffer[200];
		sprintf_s(buffer, 200, "Task posted successfully");
		utils->sendStatus(200, 1, buffer);
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
		cout << "Get all tasks - future" << endl;
	}
	else if (strcmp(meth, "POST") == 0) {
		// get the request body
		Json::Value body = utils->readRequestBody();

		if (body != NULL && !body.isNull()) {
			// choose to login or add task
			if (!body.isArray()) {
				post_login(utils, body);
			}
			else {
				post_task(utils, body);
			}
		}
	}
	else if (strcmp(meth, "DELETE") == 0) {
		cout << "Delete a task" << endl;
	}
}

// Login Credentials
//{
//	"computer_name": "desktop-usoaaoq",
//		"username" : "desktop-usoaaoq\\kannan",
//		"password" : "zoho"
//}
// 
// Post Task
//[
//{
//	"computer_name": "desktop-usoaaoq",
//		"username" : "desktop-usoaaoq\\kannan",
//		"password" : "zoho"
//},
//  {
//	"title": "Thunder",
//	"description" : "client testing",
//	"start_time" : "********165500.000000+330",
//	"run_repeat" : true,
//	"days_of_week" : 18,
//	"days_of_month" : 1048576
//  }
//]