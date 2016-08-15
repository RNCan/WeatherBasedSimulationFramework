#pragma once

#include "Basic/ERMsg.h"
#include "hxGrid/Interface/IGridUser.h"
#include "hxGrid/Interface/hxgridInterface.h"
#include "hxGrid/Common/TGenericStream.h"
#include "hxGrid/Common/MultiAppSync.h"

ERMsg GetConnectionStatus(IGridUser* pGridUser, int& loop);

