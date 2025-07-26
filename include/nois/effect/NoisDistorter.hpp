#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

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

	virtual f32_t GetPreGainDb() = 0;
	virtual void SetPreGainDb(f32_t preGainDb) = 0;

	virtual f32_t GetThresholdGainDb() = 0;
	virtual void SetThresholdGainDb(f32_t thresholdGainDb) = 0;

	virtual count_t GetNumFolds() = 0;
	virtual void SetNumFolds(count_t numFolds) = 0;

	virtual FolderFunc GetFolderFunc() = 0;
	virtual void SetFolderFunc(FolderFunc folderFunc) = 0;
};

Ref_t<Distorter<FolderDistorter>> CreateFolderDistorter(
	Ref_t<Stream> stream,
	FolderDistorter::FolderFunc folderFunc = FolderDistorter::FolderFunc::k_FolderFuncBasic);

}
