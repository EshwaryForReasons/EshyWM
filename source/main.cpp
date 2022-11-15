#include "eshywm.h"

#include <glog/logging.h>

int main(int argc, char** argv)
{
    google::InitGoogleLogging(argv[0]);
    return EshyWM::Get().initialize();
}