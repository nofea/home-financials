#include "ui_manager.hpp"
#include <iostream>

UIManager::UIManager()
{
}

UIManager::~UIManager()
{
}

void UIManager::showError(commons::Result res)
{
	if (res == commons::Result::Ok) 
    {
		return; // nothing to show for success
	}

	std::cerr << errorMessage(res) << std::endl;
}

std::string UIManager::errorMessage(commons::Result res)
{
	switch (res) 
    {
		case commons::Result::Ok:
			return "Success.";
		case commons::Result::InvalidInput:
			return "Invalid input: please check the data you provided and try again.";
		case commons::Result::NotFound:
			return "Not found: the requested family/member does not exist.";
		case commons::Result::DbError:
			return "Internal error: data storage operation failed. Try again or contact support.";
		default:
			return "An unknown error occurred.";
	}
}
