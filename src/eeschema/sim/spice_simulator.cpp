/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "ngspice.h"

#include <confirm.h>

std::shared_ptr<SPICE_SIMULATOR> SPICE_SIMULATOR::CreateInstance( const std::string& )
{
    try
    {
        static std::shared_ptr<SPICE_SIMULATOR> ngspiceInstance;

        if( !ngspiceInstance )
            ngspiceInstance.reset( new NGSPICE );

        return ngspiceInstance;
    }
    catch( std::exception& e )
    {
        DisplayError( NULL, e.what() );
    }

    return NULL;
}
