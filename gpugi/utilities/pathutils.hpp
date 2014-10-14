#pragma once

#include <string>

namespace PathUtils
{
	/// Concats to paths.
	std::string AppendPath(const std::string& _leftPath, const std::string& _rightPath);

	/// Resolves relative paths as far as possible without making them absolute,
	/// replaces backslashes with slashes, removes double slashes.
	std::string CanonicalizePath(const std::string& _path);

	/// Returns beginning of filename part of a part. std::string::npos if no filename found.
	/// \param _path Canonicalized path.
	size_t GetFilenameStart(const std::string& _path);

	/// Returns the filename of a given path.
	/// \param _path Canonicalized path.
	std::string GetFilename(const std::string& _path);

	/// Returns the directory of a given path.
	/// \param _path Canonicalized path.
	std::string GetDirectory(const std::string& _path);

} // PathUtils