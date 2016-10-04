#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "log.hpp"

namespace ngn {
    //TODO: Make getResource and others communicate with ResourceHandles to indicate deletion of resources
    //      ---> Or check if the current design makes it impossible to have a ResourceHandle on a non-existent resource
    //      (except of course if someone manually deletes it - which could also be handled by the Resource-destructor)
    //TODO: Also for asynchronous loading, so that the resource handle will either return a default resource or
    //      something user-specified (e.g. lower LOD version) as long as the real resource is loading
    //TODO: If asynchronous reloading is implemented, implement hot reloading (check periodically if file changed, but not too many at the same time)
    //      And apply effectively the same thing just that the default resource is the old resource and the new one is the modified version

    class Resource {
    private:
        static std::unordered_map<std::string, Resource*> resourceCache;

        int mReferenceCount;

    public:
        // With this function you transfer ownership of the resource to the engine and it will be collected (free'd) if no handles are pointing to it
        template<class T, typename... Args>
        static void add(const char* name, T* res) {
            auto it = resourceCache.find(name);
            if(it == resourceCache.end()) {
                resourceCache.insert(std::make_pair(name, reinterpret_cast<Resource*>(res)));
            } else {
                LOG_ERROR("Resource with name '%s' already loaded!", name);
            }
        }

        template<class T, typename... Args>
        static void prepare(const char* filename, const char* name, Args&&... args) {
            auto it = resourceCache.find(name);
            if(it == resourceCache.end()) {
                // fromFile might be derived, therefore we cast
                T* temp = static_cast<T*>(T::fromFile(filename, std::forward<Args>(args)...));
                if(!temp) {
                    LOG_ERROR("'%s' could not be loaded.", filename);
                } else {
                    resourceCache.insert(std::make_pair(name, reinterpret_cast<Resource*>(temp)));
                }
            } else {
                LOG_ERROR("Resource with name '%s' already loaded!", name);
            }
        }

        template<class T>
        static T* get(const char* name) {
            auto it = resourceCache.find(name);
            if(it == resourceCache.end()) {
                LOG_ERROR("Resource with name '%s' unknown!", name);
                return nullptr;
            } else {
                return reinterpret_cast<T*>(it->second);
            }
        }

        template<class T, typename... Args>
        static T* getPrepare(const char* filename, Args&&... args) {
            auto it = resourceCache.find(filename);
            if(it == resourceCache.end()) {
                // fromFile might be derived, therefore we cast
                T* temp = static_cast<T*>(T::fromFile(filename, std::forward<Args>(args)...));
                if(!temp) {
                    LOG_ERROR("'%s' could not be loaded.", filename);
                } else {
                    resourceCache.insert(std::make_pair(filename, reinterpret_cast<Resource*>(temp)));
                }
                return temp;
            } else {
                return reinterpret_cast<T*>(it->second);
            }
        }

        static bool collect(const char* name) {
            auto it = resourceCache.find(name);
            if(it == resourceCache.end()) return false;
            if(it->second->getReferenceCount() > 0) return false;
            delete it->second;
            return true;
        }

        static int collectAll() {
            int collected = 0;
            for(auto& it : resourceCache) {
                if(it.second->getReferenceCount() == 0) {
                    delete it.second;
                    collected++;
                }
            }
            return collected;
        }

        // Every class derived from Resource must also provide a static member of it's pointer type, containing a fallback resource
        //static Resource* fallback;

        Resource() : mReferenceCount(0) {}
        virtual ~Resource() {}

        void claim() {mReferenceCount++;}
        void release() {mReferenceCount--;}
        int getReferenceCount() const {return mReferenceCount;}
    };

    /*
    This is just if you want to make sure that some resources are not free'd, when you call collectResources()
    For example the player and common enemy models you probably always want to have in memory and the geometry of the last level you want to release
    This essentially locks resources in memory collectively (should maybe be called ResourceLock/ResourceGroupLock?) and might be super useless
    */
    class ResourceGroup {
    private:
        std::vector<Resource*> mResources;

    public:
        ~ResourceGroup() {release();}

        void claim(Resource* res) {
            res->claim();
            mResources.push_back(res);
        }

        void release() {
            for(auto res : mResources) res->release();
            mResources.clear();
        }
    };

    template<class T>
    class ResourceHandle {
    private:
        T* mResource;
        mutable bool mDirty;

    public:
        ResourceHandle(T* res = nullptr) : mResource(res), mDirty(true) {
            if(mResource) mResource->claim();
        }

        ResourceHandle(const char* filename, const char* name = nullptr) : ResourceHandle(Resource::getPrepare<T>(filename, name)) {}

        ResourceHandle(ResourceHandle&& other) : mResource(other.mResource), mDirty(true) {
            // this claim and other release cancel each other
            other.mResource = nullptr;
        }

        ResourceHandle(const ResourceHandle& other) : mResource(other.mResource), mDirty(true) {
            if(mResource) mResource->claim();
        }

        ~ResourceHandle() {
            if(mResource) mResource->release();
        }

        ResourceHandle& operator=(const ResourceHandle& other) {
            if(mResource) mResource->release();
            mResource = other.mResource;
            if(mResource) mResource->claim();
            mDirty = true;
            return *this;
        }

        ResourceHandle& operator=(ResourceHandle&& other) {
            mResource = other.mResource;
            other.mResource = nullptr;
            mDirty = true;
            return *this;
        }

        void release() {
            if(mResource) mResource->release();
            mResource = nullptr;
            mDirty = true;
        }

        bool loaded() const {
            return mResource != nullptr;
        }

        bool dirty() const {
            bool temp = mDirty;
            mDirty = false;
            return temp;
        }

        T* getResource() {
            // Should be dynamic_cast, but I don't want RTTI :/
            if(!mResource) return T::fallback;
            return mResource;
        }

        const T* getResource() const {
            if(!mResource) return T::fallback;
            return mResource;
        }
    };
}