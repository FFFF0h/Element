
#include "session/UnlockStatus.h"
#include "Globals.h"
#include "Settings.h"

#define EL_PRODUCT_ID "net.kushview.Element"
#define EL_LICENSE_SETTINGS_KEY "L"

#if EL_USE_LOCAL_AUTH
 #define EL_AUTH_URL "http://kushview.dev/products/authorize"
 #define EL_PUBKEY "11,785384ff65886b9e03e5c6c12bf46bace50be4e8183473f9a7f1fdf6a6c53445f1ea296f75576c72061e5822b70f0a8dc8fe1901f34e3eeb83f2013aa89a88fbd3f0ec68c6057019d397a50d9818f473cb99aea1d5aa1af1452e45095e5602a73112883364d2fa1fff872c9109b0e3436a881463b02a76e525b9c72dd8088dbbe3048d4b856e3623c4968200986840f650878851d03ee3cd43166f595ec121b11f46819a280864f941dd89e1b125f8b9c87dc11e7a76c92f13e6405242ba791ec19a0346f2c064bcea495da1268c567b8f6d67fad140c3069aa42f6005f7edf0181a226cfe18acf942adf72a4eb678bf142341041b06cf5d26458cbac77c75c5"

#else
 #define EL_AUTH_URL "https://kushview.net/products/authorize"
 #define EL_PUBKEY "fake,pubkey"
#endif

namespace Element {
    UnlockStatus::UnlockStatus (Globals& g) : settings (g.getSettings()) { }
    String UnlockStatus::getProductID() { return EL_PRODUCT_ID; }
    bool UnlockStatus::doesProductIDMatch (const String& returnedIDFromServer)
    {
        return getProductID() == returnedIDFromServer;
    }
    
    RSAKey UnlockStatus::getPublicKey() { return RSAKey (EL_PUBKEY); }
    
    void UnlockStatus::saveState (const String& state)
    {
        if (auto* const props = settings.getUserSettings())
            props->setValue (EL_LICENSE_SETTINGS_KEY, state);
    }
    
    String UnlockStatus::getState()
    {
        if (auto* const props = settings.getUserSettings())
            return props->getValue (EL_LICENSE_SETTINGS_KEY);
        return String();
    }
    
    String UnlockStatus::getWebsiteName()
    {
        return "kushview.net";
    }
    
    URL UnlockStatus::getServerAuthenticationURL()
    {
        const URL authurl (EL_AUTH_URL);
        return authurl;
    }
    
    String UnlockStatus::readReplyFromWebserver (const String& email, const String& password)
    {
        const URL url (getServerAuthenticationURL()
            .withParameter ("product", getProductID())
            .withParameter ("email", email)
            .withParameter ("password", password)
            .withParameter ("os", SystemStats::getOperatingSystemName())
            .withParameter ("mach", getLocalMachineIDs().joinIntoString(",")));
        
        DBG ("Trying to unlock via: " << url.toString (true));
        return url.readEntireTextStream();
    }
    
    StringArray UnlockStatus::getLocalMachineIDs()
    {
        auto ids (OnlineUnlockStatus::getLocalMachineIDs());
        return ids;
    }
}