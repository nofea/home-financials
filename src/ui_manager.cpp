#include "ui_manager.hpp"
#include <iostream>

/**
 * @brief Construct a new UIManager::UIManager object
 * 
 */
UIManager::UIManager()
{
}

/**
 * @brief Destroy the UIManager::UIManager object
 * 
 */
UIManager::~UIManager()
{
}

/**
 * @brief Show an error message to the user
 * 
 * @param res The result containing error information
 */
void UIManager::showError(const commons::Result& res)
{
	if (res == commons::Result::Ok) 
	{
		return; // nothing to show for success
	}

	std::cerr << errorMessage(res) << std::endl;
}

/**
 * @brief Translate an error code to a human-friendly message.
 * 
 * @param res The result containing error information	
 * @return std::string 
 */
std::string UIManager::errorMessage(const commons::Result& res)
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
