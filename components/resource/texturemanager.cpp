#include "texturemanager.hpp"

#include <osgDB/Registry>

#include <stdexcept>

#include <components/vfs/manager.hpp>

namespace
{

    osg::ref_ptr<osg::Texture2D> createWarningTexture()
    {
        osg::ref_ptr<osg::Image> warningImage = new osg::Image;

        int width = 8, height = 8;
        warningImage->allocateImage(width, height, 1, GL_RGB, GL_UNSIGNED_BYTE);
        assert (warningImage->isDataContiguous());
        unsigned char* data = warningImage->data();
        for (int i=0;i<width*height;++i)
        {
            data[3*i] = (255);
            data[3*i+1] = (0);
            data[3*i+2] = (255);
        }

        osg::ref_ptr<osg::Texture2D> warningTexture = new osg::Texture2D;
        warningTexture->setImage(warningImage);
        return warningTexture;
    }

}

namespace Resource
{

    TextureManager::TextureManager(const VFS::Manager *vfs)
        : mVFS(vfs)
        , mWarningTexture(createWarningTexture())
    {

    }

    /*
    osg::ref_ptr<osg::Image> TextureManager::getImage(const std::string &filename)
    {

    }
    */

    osg::ref_ptr<osg::Texture2D> TextureManager::getTexture2D(const std::string &filename, osg::Texture::WrapMode wrapS, osg::Texture::WrapMode wrapT)
    {
        std::string normalized = filename;
        mVFS->normalizeFilename(normalized);
        MapKey key = std::make_pair(std::make_pair(wrapS, wrapT), normalized);
        std::map<MapKey, osg::ref_ptr<osg::Texture2D> >::iterator found = mTextures.find(key);
        if (found != mTextures.end())
        {
            return found->second;
        }
        else
        {
            Files::IStreamPtr stream;
            try
            {
                stream = mVFS->get(normalized.c_str());
            }
            catch (std::exception& e)
            {
                std::cerr << "Failed to open texture: " << e.what() << std::endl;
                return mWarningTexture;
            }

            osg::ref_ptr<osgDB::Options> opts (new osgDB::Options);
            opts->setOptionString("dds_dxt1_detect_rgba"); // tx_creature_werewolf.dds isn't loading in the correct format without this option
            size_t extPos = normalized.find_last_of('.');
            std::string ext;
            if (extPos != std::string::npos && extPos+1 < normalized.size())
                ext = normalized.substr(extPos+1);
            osgDB::ReaderWriter* reader = osgDB::Registry::instance()->getReaderWriterForExtension(ext);
            osgDB::ReaderWriter::ReadResult result = reader->readImage(*stream, opts);
            if (!result.success())
            {
                std::cerr << "Error loading " << filename << ": " << result.message() << std::endl;
                return mWarningTexture;
            }

            osg::Image* image = result.getImage();
            osg::ref_ptr<osg::Texture2D> texture(new osg::Texture2D);
            texture->setImage(image);
            texture->setWrap(osg::Texture::WRAP_S, wrapS);
            texture->setWrap(osg::Texture::WRAP_T, wrapT);

            // Can be enabled for single-context, i.e. in openmw
            //texture->setUnRefImageDataAfterApply(true);

            mTextures.insert(std::make_pair(key, texture));
            return texture;
        }
    }

}
