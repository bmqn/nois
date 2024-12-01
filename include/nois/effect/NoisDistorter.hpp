#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

#include <memory>

namespace nois {

template<typename T>
class Distorter : public Stream
{
public:
	virtual ~Distorter() {}

	T &GetDistorter();
};

class FolderDistorter : public Distorter<FolderDistorter>
{
public:
	enum FolderFunc
	{
		k_FolderFuncBasic,
		k_FolderFuncRizzler
	};

public:
	virtual ~FolderDistorter() {}

	virtual data_t GetPreGainDb() = 0;
	virtual void SetPreGainDb(data_t preGainDb) = 0;

	virtual data_t GetThresholdGainDb() = 0;
	virtual void SetThresholdGainDb(data_t thresholdGainDb) = 0;

	virtual count_t GetNumFolds() = 0;
	virtual void SetNumFolds(count_t numFolds) = 0;

	virtual FolderFunc GetFolderFunc() = 0;
	virtual void SetFolderFunc(FolderFunc folderFunc) = 0;
};

std::shared_ptr<Distorter<FolderDistorter>> CreateFolderDistorter(Stream *stream,
	FolderDistorter::FolderFunc folderFunc = FolderDistorter::FolderFunc::k_FolderFuncBasic);

}
