#include "pch.h"
#include "AssetSystem.h"

#include <AssetManager.h>
#include <filesystem>

#include <IO/Utility.h>

namespace nv::asset
{
    class FileWatcher
    {
        using FileMap = HashMap<std::string, std::filesystem::file_time_type>;
    public:
        FileWatcher(const char* path, float delay) :
            mPath(path),
            mDelay(delay),
            mCurrTime(0)
        {}

        void Init()
        {
            for (auto& file : std::filesystem::recursive_directory_iterator(mPath))
            {
                if(file.is_regular_file())
                    mFiles[file.path().string()] = std::filesystem::last_write_time(file);
            }
        }

        void DetectModifiedFiles(float delta)
        {
            mModifiedFiles.Clear();
            mCurrTime += delta;
            if (mCurrTime < mDelay)
                return;

            mCurrTime = 0;

            const auto contains = [this](std::string& path)
            {
                return mFiles.find(path) != mFiles.end();
            };

            for (auto& file : std::filesystem::recursive_directory_iterator(mPath)) 
            {
                auto fileLastWriteTime = std::filesystem::last_write_time(file); 
                
                auto pathStr = file.path().string();
                if (contains(pathStr))
                {
                    auto diff = fileLastWriteTime - mFiles[pathStr];
                    if (mFiles[pathStr] < fileLastWriteTime)
                    {
                        mFiles[pathStr] = fileLastWriteTime;
                        io::NormalizePath(pathStr);
                        mModifiedFiles.Push(pathStr);
                    }
                }
            }
        }

        nv::Span<std::string> GetModifiedFiles()
        {
            return mModifiedFiles.Span();
        }

    private:
        FileMap                 mFiles;
        std::string             mPath;
        nv::Vector<std::string> mModifiedFiles;
        float                   mDelay;
        float                   mCurrTime;
    };

    constexpr float FILE_WATCH_DELAY_SECONDS = 1.f;
    static FileWatcher gFileWatcher(RAW_ASSET_PATH, FILE_WATCH_DELAY_SECONDS);

    void AssetSystem::Init()
    {
        gFileWatcher.Init();
    }

    void AssetSystem::Update(float deltaTime, float totalTime)
    {
        gpAssetManager->Tick();
        gFileWatcher.DetectModifiedFiles(deltaTime);
        auto modifiedFiles = gFileWatcher.GetModifiedFiles();
        for (const auto& modified : modifiedFiles)
        {
            gpAssetManager->Reload(modified.c_str());
        }
    }

    void AssetSystem::Destroy()
    {
    }

    void AssetSystem::OnReload()
    {
    }
}
