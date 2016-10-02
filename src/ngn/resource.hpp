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
        // Use these to preload as well
        template<class T>
        static T* get(const char* filename, const char* name = nullptr) {
            if(!name) name = filename;

            auto it = resourceCache.find(name);
            if(it == resourceCache.end()) {
                T* temp = new T;
                if(!temp->load(filename)) {
                    LOG_ERROR("'%s' could not be loaded.", filename);
                    return nullptr;
                }
                resourceCache.insert(std::make_pair(name, reinterpret_cast<Resource*>(temp)));
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

        virtual bool load(const char* filename) = 0;
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

    public:
        ResourceHandle(T* res = nullptr) : mResource(res) {
            if(mResource) mResource->claim();
        }

        template<typename ...Args>
        ResourceHandle(Args&&... args) : ResourceHandle(Resource::get(std::forward<Args>(args)...)) {}

        ResourceHandle(const ResourceHandle& other) : mResource(other.mResource) {
            if(mResource) mResource->claim();
        }

        ResourceHandle(ResourceHandle&& other) : mResource(other.mResource) {
            // this claim and other release cancel each other
            other.mResource = nullptr;
        }

        ~ResourceHandle() {
            if(mResource) mResource->release();
        }

        ResourceHandle& operator=(const ResourceHandle& other) {
            if(mResource) mResource->release();
            mResource = other.mResource;
            if(mResource) mResource->claim();
            return *this;
        }

        ResourceHandle& operator=(ResourceHandle&& other) {
            mResource = other.mResource;
            other.mResource = nullptr;
            return *this;
        }

        void release() {
            if(mResource) mResource->release();
            mResource = nullptr;
        }

        bool loaded() const {
            return mResource != nullptr;
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