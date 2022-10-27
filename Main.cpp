#pragma once

#include <iostream>
#include <stdlib.h>
#include <string>

#include <json/json.h>

#include "Utils.h";

using namespace std;

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
				cout << "Login" << endl;
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