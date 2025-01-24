#include "fileListing.hpp"

#include <iostream>
#include <algorithm>

#include "include/ghc/fs_std.hpp"


#include "code/Region/RegionManager.hpp"
#include "common/NBT.hpp"


namespace editor {

    void FileListing::printDetails() const {
        printf("\n** FileListing Details **\n");
        printf("1. Filename: %s\n", myReadSettings.getFilePath().string().c_str());
        printf("2. Oldest  Version: %d\n", myReadSettings.getOldestVersion());
        printf("3. Current Version: %d\n", myReadSettings.getCurrentVersion());
        printf("4. Total  File Count: %zu\n", myAllFiles.size());
        printf("5. Player File Count: %zu\n", ptrs.players.size());
        printFileList();
    }


    void FileListing::printFileList() const {
        printf("\n** Files Contained **\n");
        int index = 0;
        for (c_auto& myAllFile : myAllFiles) {
            printf("%.2d [%7d]: %s\n", index, myAllFile.data.size,
                   myAllFile.constructFileName(myReadSettings.getConsole(), myReadSettings.getHasSepRegions()).c_str());
            index++;
        }
        printf("\n");
    }


    /**
     * Pass in the path that you want "dump/CONSOLE" to be made in.
     * @param inDirPath
     * @return
     */
    int FileListing::dumpToFolder(const fs::path& inDirPath) const {
        const fs::path consoleDirPath = inDirPath / ("dump/" + consoleToStr(myReadSettings.getConsole()));

        // deletes all files in "DIR/dump/CONSOLE/".
        if (exists(consoleDirPath) && is_directory(consoleDirPath)) {
            for (c_auto &entry: fs::directory_iterator(consoleDirPath)) {
                try {
                    remove_all(entry.path());
                } catch (const fs::filesystem_error &e) {
                    std::cerr << "Filesystem error: " << e.what() << '\n';
                }
            }
        }

        // puts each file in "DIR/dump/CONSOLE/".
        for (const LCEFile &file: myAllFiles) {
            const fs::path fullFilePath = consoleDirPath
                / file.constructFileName(myReadSettings.getConsole(), myReadSettings.getHasSepRegions());

            // makes folders (such as "data") in "DIR/dump/CONSOLE/" if they do not exist
            if (!exists(fullFilePath.parent_path())) {
                create_directories(fullFilePath.parent_path());
            }

            // writes the file to "DIR/dump/CONSOLE/FILENAME".
            DataManager fileOut(file.data);
            if (fileOut.writeToFile(fullFilePath)) {
                return FILE_ERROR;
            }
        }

        return SUCCESS;
    }


    std::list<LCEFile> FileListing::collectFiles(lce::FILETYPE fileType) {
        std::list<LCEFile> collectedFiles;

        for (auto it = myAllFiles.begin(); it != myAllFiles.end(); ) {
            if (it->fileType == fileType) {
                collectedFiles.splice(collectedFiles.end(), myAllFiles, it++);
            } else {
                ++it;
            }
        }

        ptrs.clearRemove[fileType]();
        return collectedFiles;
    }


    void FileListing::deallocate() {
        for (LCEFile& file : myAllFiles) {
            file.deleteData();
        }
        clearPointers();
        myAllFiles.clear();
        myReadSettings.reset();
    }


    void FileListing::clearPointers() {
        ptrs.region_overworld.clear();
        ptrs.region_nether.clear();
        ptrs.region_end.clear();
        ptrs.entity_overworld = nullptr;
        ptrs.entity_nether = nullptr;
        ptrs.entity_end = nullptr;
        ptrs.maps.clear();
        ptrs.structures.clear();
        ptrs.players.clear();
        ptrs.largeMapDataMappings = nullptr;
        ptrs.level = nullptr;
        ptrs.grf = nullptr;
        ptrs.village = nullptr;
    }


    void FileListing::updatePointers() {
        clearPointers();
        for (LCEFile& file : myAllFiles) {
            auto it = ptrs.addUpdate.find(file.fileType);
            if (it != ptrs.addUpdate.end()) {
                it->second(file);
            }
        }
    }


    // TODO: this does not work at all
    // TODO: this should be popping the current node, not the ending node
    void FileListing::removeFileTypes(const std::set<lce::FILETYPE>& typesToRemove) {
        auto iter = myAllFiles.begin();
        while (iter != myAllFiles.end()) {
            if (typesToRemove.contains(iter->fileType)) {
                iter->deleteData();
                iter = myAllFiles.erase(iter);
            } else {
                ++iter;
            }
        }
        for (c_auto& fileType : typesToRemove) {
            ptrs.clearRemove[fileType]();
        }

        updatePointers();
    }


    MU void FileListing::addFiles(std::list<LCEFile> filesIn) {
        myAllFiles.splice(myAllFiles.end(), filesIn);
        updatePointers();
    }


    MU void FileListing::convertRegions(const lce::CONSOLE consoleOut) {
        // int index = 0;
        // int index2 = 0;
        for (const FileList* fileList : ptrs.dimFileLists) {
            // printf("list %d\n", index);
            // index++;
            // index2 = 0;
            for (LCEFile* file : *fileList) {
                // printf("    file %d\n", index2);
                // index2++;



                // don't convert it if it's already the correct console version
                // if (file->console == consoleOut) {
                //     continue;
                // }
                RegionManager region;
                region.read(file);
                region.convertChunks(consoleOut);
                Data data = region.write(consoleOut);
                file->data.steal(data);
                file->console = consoleOut;
            }
        }
    }


    MU ND int FileListing::convertTo(const fs::path& inFilePath,
                                     const fs::path& outFilePath,
                                     lce::CONSOLE consoleOut) {
        int status = findConsole(inFilePath);
        if (status != SUCCESS) {
            return status;
        }

        (void) dumpToFolder("");

        removeFileTypes({lce::FILETYPE::PLAYER, lce::FILETYPE::DATA_MAPPING});

        convertRegions(consoleOut);

        WriteSettings settings(consoleOut, outFilePath);
        status = write(settings);
        return status;
    }


    MU ND int FileListing::convertAndReplaceRegions(const fs::path& inFilePath,
                                                    const fs::path& inFileRegionReplacementPath,
                                                    const fs::path& outFilePath, const lce::CONSOLE consoleOut) {

        int status = read(inFilePath);
        if (status != SUCCESS) { return status; }

        FileListing replace;
        status = replace.read(inFileRegionReplacementPath);
        if (status != SUCCESS) { return status; }

        removeFileTypes({lce::FILETYPE::REGION_NETHER,
                         lce::FILETYPE::REGION_OVERWORLD,
                         lce::FILETYPE::REGION_END});


        addFiles(replace.collectFiles(lce::FILETYPE::REGION_NETHER));
        addFiles(replace.collectFiles(lce::FILETYPE::REGION_OVERWORLD));
        addFiles(replace.collectFiles(lce::FILETYPE::REGION_END));

        convertRegions(consoleOut);

        replace.deallocate();

        WriteSettings settings(consoleOut, outFilePath);
        status = write(settings);
        return status;
    }


    void FileListing::pruneRegions() {
        for (auto iter = myAllFiles.begin(); iter != myAllFiles.end(); ) {
            if (iter->isRegionType()) {
                c_i16 regionX = iter->getRegionX();
                c_i16 regionZ = iter->getRegionZ();
                if (!(regionX == 0 || regionX == -1) || !(regionZ == 0 || regionZ == -1)) {
                    iter->deleteData();
                    iter = myAllFiles.erase(iter);
                    continue;
                }
            }
            ++iter;
        }
        clearPointers();
        updatePointers();
    }


    MU void FileListing::replaceRegionOW(size_t regionIndex, editor::RegionManager& region, const lce::CONSOLE consoleOut) {
        if (regionIndex >= ptrs.region_overworld.size()) {
            throw std::runtime_error(
                "attempted to call FileListing::replaceRegionOW with an index that is out of bounds.");
        }
        ptrs.region_overworld[regionIndex]->data.deallocate();
        ptrs.region_overworld[regionIndex]->data = region.write(consoleOut);
    }


}