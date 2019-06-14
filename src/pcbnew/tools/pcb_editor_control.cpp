/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2014-2019 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#include <cstdint>
#include <thread>
#include <functional>
#include "pcb_editor_control.h"
#include "pcb_actions.h"
#include <tool/tool_manager.h>
#include <tools/tool_event_utils.h>
#include <wx/progdlg.h>
#include <ws_proxy_undo_item.h>
#include "edit_tool.h"
#include "selection_tool.h"
#include "drawing_tool.h"
#include "pcbnew_picker_tool.h"

#include <painter.h>
#include <project.h>
#include <pcbnew_id.h>
#include <pcb_edit_frame.h>
#include <class_board.h>
#include <class_zone.h>
#include <pcb_draw_panel_gal.h>
#include <class_module.h>
#include <class_pcb_target.h>
#include <connectivity/connectivity_data.h>
#include <collectors.h>
#include <zones_functions_for_undo_redo.h>
#include <board_commit.h>
#include <confirm.h>
#include <bitmaps.h>
#include <view/view_group.h>
#include <view/view_controls.h>
#include <origin_viewitem.h>
#include <profile.h>
#include <widgets/progress_reporter.h>
#include <dialogs/dialog_find.h>
#include <dialogs/dialog_page_settings.h>
#include <pcb_netlist.h>
#include <dialogs/dialog_update_pcb.h>
#include <gestfich.h>
#include <wildcards_and_files_ext.h>

using namespace std::placeholders;


TOOL_ACTION PCB_ACTIONS::boardSetup( "pcbnew.EditorControl.boardSetup",
        AS_GLOBAL, 0, "",
        _( "Board Setup..." ), 
        _( "Edit board setup including layers, design rules and various defaults" ),
        options_board_xpm );

TOOL_ACTION PCB_ACTIONS::importNetlist( "pcbnew.EditorControl.importNetlist",
        AS_GLOBAL, 0, "",
        _( "Netlist..." ), _( "Read netlist and update board connectivity" ),
        netlist_xpm );

TOOL_ACTION PCB_ACTIONS::importSpecctraSession( "pcbnew.EditorControl.importSpecctraSession",
        AS_GLOBAL, 0, "",
        _( "Specctra Session..." ), _( "Import routed Specctra session (*.ses) file" ),
        import_xpm );

TOOL_ACTION PCB_ACTIONS::exportSpecctraDSN( "pcbnew.EditorControl.exportSpecctraDSN",
        AS_GLOBAL, 0, "",
        _( "Specctra DSN..." ), _( "Export Specctra DSN routing info" ),
        export_dsn_xpm );

TOOL_ACTION PCB_ACTIONS::generateGerbers( "pcbnew.EditorControl.generateGerbers",
        AS_GLOBAL, 0, "",
        _( "&Gerbers (.gbr)..." ), _( "Generate Gerbers for fabrication" ),
        post_compo_xpm );

TOOL_ACTION PCB_ACTIONS::generateDrillFiles( "pcbnew.EditorControl.generateDrillFiles",
        AS_GLOBAL, 0, "",
        _( "&Drill Files (.drl)..." ), _( "Generate Excellon drill file(s)" ),
        post_drill_xpm );

TOOL_ACTION PCB_ACTIONS::generatePosFile( "pcbnew.EditorControl.generatePosFile",
        AS_GLOBAL, 0, "",
        _( "Footprint &Positions (.pos)..." ),
        _( "Generate footprint position file for pick and place" ),
        post_compo_xpm );

TOOL_ACTION PCB_ACTIONS::generateReportFile( "pcbnew.EditorControl.generateReportFile",
        AS_GLOBAL, 0, "",
        _( "&Footprint Report (.rpt)..." ),
        _( "Create report of all footprints from current board" ),
        tools_xpm );

TOOL_ACTION PCB_ACTIONS::generateD356File( "pcbnew.EditorControl.generateD356File",
        AS_GLOBAL, 0, "",
        _( "IPC-D-356 Netlist File..." ), _( "Generate IPC-D-356 netlist file" ),
        netlist_xpm );

TOOL_ACTION PCB_ACTIONS::generateBOM( "pcbnew.EditorControl.generateBOM",
        AS_GLOBAL, 0, "",
        _( "&BOM..." ), _( "Create bill of materials from current schematic" ),
        bom_xpm );

// Track & via size control
TOOL_ACTION PCB_ACTIONS::trackWidthInc( "pcbnew.EditorControl.trackWidthInc",
        AS_GLOBAL, 
        'W', LEGACY_HK_NAME( "Switch Track Width To Next" ),
        "", "" );

TOOL_ACTION PCB_ACTIONS::trackWidthDec( "pcbnew.EditorControl.trackWidthDec",
        AS_GLOBAL, 
        MD_SHIFT + 'W', LEGACY_HK_NAME( "Switch Track Width To Previous" ),
        "", "" );

TOOL_ACTION PCB_ACTIONS::viaSizeInc( "pcbnew.EditorControl.viaSizeInc",
        AS_GLOBAL,
        '\'', LEGACY_HK_NAME( "Increase Via Size" ),
        "", "" );

TOOL_ACTION PCB_ACTIONS::viaSizeDec( "pcbnew.EditorControl.viaSizeDec",
        AS_GLOBAL,
        '\\', LEGACY_HK_NAME( "Decrease Via Size" ),
        "", "" );

TOOL_ACTION PCB_ACTIONS::trackViaSizeChanged( "pcbnew.EditorControl.trackViaSizeChanged",
        AS_GLOBAL, 0, "",
        "", "",
        nullptr, AF_NOTIFY );

TOOL_ACTION PCB_ACTIONS::zoneMerge( "pcbnew.EditorControl.zoneMerge",
        AS_GLOBAL, 0, "",
        _( "Merge Zones" ), _( "Merge zones" ) );

TOOL_ACTION PCB_ACTIONS::zoneDuplicate( "pcbnew.EditorControl.zoneDuplicate",
        AS_GLOBAL, 0, "",
        _( "Duplicate Zone onto Layer..." ), _( "Duplicate zone outline onto a different layer" ),
        zone_duplicate_xpm );

TOOL_ACTION PCB_ACTIONS::placeTarget( "pcbnew.EditorControl.placeTarget",
        AS_GLOBAL, 0, "",
        _( "Add Layer Alignment Target" ), _( "Add a layer alignment target" ),
        add_pcb_target_xpm, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::placeModule( "pcbnew.EditorControl.placeModule",
        AS_GLOBAL, 
        'O', LEGACY_HK_NAME( "Add Footprint" ),
        _( "Add Footprint" ), _( "Add a footprint" ),
        module_xpm, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drillOrigin( "pcbnew.EditorControl.drillOrigin",
        AS_GLOBAL, 0, "",
        _( "Drill and Place Offset" ), _( "Place origin point for drill and place files" ),
        pcb_offset_xpm );

TOOL_ACTION PCB_ACTIONS::crossProbeSchToPcb( "pcbnew.EditorControl.crossProbSchToPcb",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::toggleLock( "pcbnew.EditorControl.toggleLock",
        AS_GLOBAL, 
        'L', LEGACY_HK_NAME( "Lock/Unlock Footprint" ),
        "Toggle Lock", "",
        lock_unlock_xpm );

TOOL_ACTION PCB_ACTIONS::lock( "pcbnew.EditorControl.lock",
        AS_GLOBAL, 0, "",
        _( "Lock" ), "",
        locked_xpm );

TOOL_ACTION PCB_ACTIONS::unlock( "pcbnew.EditorControl.unlock",
        AS_GLOBAL, 0, "",
        _( "Unlock" ), "",
        unlocked_xpm );

TOOL_ACTION PCB_ACTIONS::appendBoard( "pcbnew.EditorControl.appendBoard",
        AS_GLOBAL, 0, "",
        "Append Board...", "", 
        add_board_xpm );

TOOL_ACTION PCB_ACTIONS::highlightNet( "pcbnew.EditorControl.highlightNet",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::clearHighlight( "pcbnew.EditorControl.clearHighlight",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::highlightNetTool( "pcbnew.EditorControl.highlightNetTool",
        AS_GLOBAL, 0, "",
        _( "Highlight Nets" ), _( "Highlight all copper items of a net" ),
        net_highlight_xpm );

TOOL_ACTION PCB_ACTIONS::highlightNetSelection( "pcbnew.EditorControl.highlightNetSelection",
        AS_GLOBAL,
        '`', LEGACY_HK_NAME( "Toggle Highlight of Selected Net (Modern Toolset only)" ),
        _( "Highlight Net" ), _( "Highlight all copper items of a net" ),
        net_highlight_xpm );

TOOL_ACTION PCB_ACTIONS::localRatsnestTool( "pcbnew.Control.localRatsnestTool",
        AS_GLOBAL, 0, "",
        _( "Highlight Ratsnest" ), "",
        tool_ratsnest_xpm );

TOOL_ACTION PCB_ACTIONS::hideDynamicRatsnest( "pcbnew.Control.hideDynamicRatsnest", 
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::updateLocalRatsnest( "pcbnew.Control.updateLocalRatsnest",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::listNets( "pcbnew.Control.listNets",
        AS_GLOBAL, 0, "",
        _( "List Nets" ), _( "Show a list of nets with names and IDs" ),
        list_nets_xpm );

TOOL_ACTION PCB_ACTIONS::showPythonConsole( "pcbnew.Control.showPythonConsole",
        AS_GLOBAL, 0, "",
        _( "Scripting Console" ), _( "Show the Python scripting console" ),
        py_script_xpm );

TOOL_ACTION PCB_ACTIONS::showLayersManager( "pcbnew.Control.showLayersManager",
        AS_GLOBAL, 0, "",
        _( "Show Layers Manager" ), _( "Show/hide the layers manager" ),
        layers_manager_xpm );

TOOL_ACTION PCB_ACTIONS::showMicrowaveToolbar( "pcbnew.Control.showMicrowaveToolbar",
        AS_GLOBAL, 0, "",
        _( "Show Microwave Toolbar" ), _( "Show/hide microwave toolbar\n(Experimental feature)" ),
        mw_toolbar_xpm );

TOOL_ACTION PCB_ACTIONS::flipBoard( "pcbnew.Control.flipBoard",
        AS_GLOBAL, 0, "",
        _( "Flip Board View" ), _( "Flip (mirror) the board view" ),
        flip_board_xpm );


class ZONE_CONTEXT_MENU : public ACTION_MENU
{
public:
    ZONE_CONTEXT_MENU()
    {
        SetIcon( add_zone_xpm );
        SetTitle( _( "Zones" ) );

        Add( PCB_ACTIONS::zoneFill );
        Add( PCB_ACTIONS::zoneFillAll );
        Add( PCB_ACTIONS::zoneUnfill );
        Add( PCB_ACTIONS::zoneUnfillAll );

        AppendSeparator();

        Add( PCB_ACTIONS::zoneMerge );
        Add( PCB_ACTIONS::zoneDuplicate );
        Add( PCB_ACTIONS::drawZoneCutout );
        Add( PCB_ACTIONS::drawSimilarZone );
    }


protected:
    ACTION_MENU* create() const override
    {
        return new ZONE_CONTEXT_MENU();
    }

private:
    void update() override
    {
        SELECTION_TOOL* selTool = getToolManager()->GetTool<SELECTION_TOOL>();

        // enable zone actions that act on a single zone
        bool singleZoneActionsEnabled = ( SELECTION_CONDITIONS::Count( 1 )
                                          && SELECTION_CONDITIONS::OnlyType( PCB_ZONE_AREA_T )
                                        )( selTool->GetSelection() );

        Enable( getMenuId( PCB_ACTIONS::zoneDuplicate ), singleZoneActionsEnabled );
        Enable( getMenuId( PCB_ACTIONS::drawZoneCutout ), singleZoneActionsEnabled );
        Enable( getMenuId( PCB_ACTIONS::drawSimilarZone ), singleZoneActionsEnabled );

        // enable zone actions that ably to a specific set of zones (as opposed to all of them)
        bool nonGlobalActionsEnabled = ( SELECTION_CONDITIONS::MoreThan( 0 ) )( selTool->GetSelection() );

        Enable( getMenuId( PCB_ACTIONS::zoneFill ), nonGlobalActionsEnabled );
        Enable( getMenuId( PCB_ACTIONS::zoneUnfill ), nonGlobalActionsEnabled );

        // lines like this make me really think about a better name for SELECTION_CONDITIONS class
        bool mergeEnabled = ( SELECTION_CONDITIONS::MoreThan( 1 ) &&
                              /*SELECTION_CONDITIONS::OnlyType( PCB_ZONE_AREA_T ) &&*/
                              PCB_SELECTION_CONDITIONS::SameNet( true ) &&
                              PCB_SELECTION_CONDITIONS::SameLayer() )( selTool->GetSelection() );

        Enable( getMenuId( PCB_ACTIONS::zoneMerge ), mergeEnabled );
    }
};


class LOCK_CONTEXT_MENU : public ACTION_MENU
{
public:
    LOCK_CONTEXT_MENU()
    {
        SetIcon( locked_xpm );
        SetTitle( _( "Locking" ) );

        AppendSeparator();
        Add( PCB_ACTIONS::lock );
        Add( PCB_ACTIONS::unlock );
        Add( PCB_ACTIONS::toggleLock );
    }

    ACTION_MENU* create() const override
    {
        return new LOCK_CONTEXT_MENU();
    }
};


PCB_EDITOR_CONTROL::PCB_EDITOR_CONTROL() :
    PCB_TOOL_BASE( "pcbnew.EditorControl" ),
    m_frame( nullptr )
{
    m_placeOrigin.reset( new KIGFX::ORIGIN_VIEWITEM( KIGFX::COLOR4D( 0.8, 0.0, 0.0, 1.0 ),
                                                KIGFX::ORIGIN_VIEWITEM::CIRCLE_CROSS ) );
    m_probingSchToPcb = false;
    m_slowRatsnest = false;
}


PCB_EDITOR_CONTROL::~PCB_EDITOR_CONTROL()
{
}


void PCB_EDITOR_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_EDIT_FRAME>();

    if( aReason == MODEL_RELOAD || aReason == GAL_SWITCH )
    {
        m_placeOrigin->SetPosition( getModel<BOARD>()->GetAuxOrigin() );
        getView()->Remove( m_placeOrigin.get() );
        getView()->Add( m_placeOrigin.get() );
    }
}


bool PCB_EDITOR_CONTROL::Init()
{
    auto activeToolCondition = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() != ID_NO_TOOL_SELECTED );
    };

    auto inactiveStateCondition = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() == ID_NO_TOOL_SELECTED && aSel.Size() == 0 );
    };

    auto placeModuleCondition = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() == ID_PCB_MODULE_BUTT && aSel.GetSize() == 0 );
    };

    auto& ctxMenu = m_menu.GetMenu();

    // "Cancel" goes at the top of the context menu when a tool is active
    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeToolCondition, 1 );
    ctxMenu.AddSeparator( activeToolCondition, 1 );

    // "Get and Place Footprint" should be available for Place Footprint tool
    ctxMenu.AddItem( PCB_ACTIONS::findMove, placeModuleCondition, 1000 );
    ctxMenu.AddSeparator( placeModuleCondition, 1000 );

    // Finally, add the standard zoom & grid items
    m_menu.AddStandardSubMenus( getEditFrame<PCB_BASE_FRAME>() );

    auto zoneMenu = std::make_shared<ZONE_CONTEXT_MENU>();
    zoneMenu->SetTool( this );

    auto lockMenu = std::make_shared<LOCK_CONTEXT_MENU>();
    lockMenu->SetTool( this );

    // Add the PCB control menus to relevant other tools

    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();

    if( selTool )
    {
        auto& toolMenu = selTool->GetToolMenu();
        auto& menu = toolMenu.GetMenu();

        // Add "Get and Place Footprint" when Selection tool is in an inactive state
        menu.AddItem( PCB_ACTIONS::findMove, inactiveStateCondition );
        menu.AddSeparator( inactiveStateCondition );

        toolMenu.AddSubMenu( zoneMenu );
        toolMenu.AddSubMenu( lockMenu );

        menu.AddMenu( zoneMenu.get(), SELECTION_CONDITIONS::OnlyType( PCB_ZONE_AREA_T ), 200 );
        menu.AddMenu( lockMenu.get(), SELECTION_CONDITIONS::OnlyTypes( GENERAL_COLLECTOR::LockableItems ), 200 );
    }

    DRAWING_TOOL* drawingTool = m_toolMgr->GetTool<DRAWING_TOOL>();

    if( drawingTool )
    {
        auto& toolMenu = drawingTool->GetToolMenu();
        auto& menu = toolMenu.GetMenu();

        toolMenu.AddSubMenu( zoneMenu );

        // Functor to say if the PCB_EDIT_FRAME is in a given mode
        // Capture the tool pointer and tool mode by value
        auto toolActiveFunctor = [=]( DRAWING_TOOL::MODE aMode )
        {
            return [=]( const SELECTION& sel )
            {
                return drawingTool->GetDrawingMode() == aMode;
            };
        };

        menu.AddMenu( zoneMenu.get(), toolActiveFunctor( DRAWING_TOOL::MODE::ZONE ), 200 );
    }

    m_ratsnestTimer.SetOwner( this );
    Connect( m_ratsnestTimer.GetId(), wxEVT_TIMER,
            wxTimerEventHandler( PCB_EDITOR_CONTROL::ratsnestTimer ), NULL, this );

    return true;
}


int PCB_EDITOR_CONTROL::New( const TOOL_EVENT& aEvent )
{
    m_frame->Files_io_from_id( ID_NEW_BOARD );
    return 0;
}


int PCB_EDITOR_CONTROL::Open( const TOOL_EVENT& aEvent )
{
    m_frame->Files_io_from_id( ID_LOAD_FILE );
    return 0;
}


int PCB_EDITOR_CONTROL::Save( const TOOL_EVENT& aEvent )
{
    m_frame->Files_io_from_id( ID_SAVE_BOARD );
    return 0;
}


int PCB_EDITOR_CONTROL::SaveAs( const TOOL_EVENT& aEvent )
{
    m_frame->Files_io_from_id( ID_SAVE_BOARD_AS );
    return 0;
}


int PCB_EDITOR_CONTROL::SaveCopyAs( const TOOL_EVENT& aEvent )
{
    m_frame->Files_io_from_id( ID_COPY_BOARD_AS );
    return 0;
}


int PCB_EDITOR_CONTROL::PageSettings( const TOOL_EVENT& aEvent )
{
    PICKED_ITEMS_LIST   undoCmd;
    WS_PROXY_UNDO_ITEM* undoItem = new WS_PROXY_UNDO_ITEM( m_frame );
    ITEM_PICKER         wrapper( undoItem, UR_PAGESETTINGS );

    undoCmd.PushItem( wrapper );
    m_frame->SaveCopyInUndoList( undoCmd, UR_PAGESETTINGS );

    DIALOG_PAGES_SETTINGS dlg( m_frame, wxSize( MAX_PAGE_SIZE_PCBNEW_MILS,
                                                MAX_PAGE_SIZE_PCBNEW_MILS ) );
    dlg.SetWksFileName( BASE_SCREEN::m_PageLayoutDescrFileName );

    if( dlg.ShowModal() == wxID_OK )
        m_toolMgr->RunAction( ACTIONS::zoomFitScreen, true );
    else
        m_frame->RollbackFromUndo();

    return 0;
}


int PCB_EDITOR_CONTROL::Plot( const TOOL_EVENT& aEvent )
{
    m_frame->ToPlotter( ID_GEN_PLOT );
    return 0;
}


int PCB_EDITOR_CONTROL::BoardSetup( const TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->DoShowBoardSetupDialog();
    return 0;
}


int PCB_EDITOR_CONTROL::ImportNetlist( const TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->InstallNetlistFrame();
    return 0;
}


int PCB_EDITOR_CONTROL::ImportSpecctraSession( const TOOL_EVENT& aEvent )
{
    wxString fullFileName = frame()->GetBoard()->GetFileName();
    wxString path;
    wxString name;
    wxString ext;

    wxFileName::SplitPath( fullFileName, &path, &name, &ext );
    name += wxT( ".ses" );

    fullFileName = EDA_FILE_SELECTOR( _( "Merge Specctra Session file:" ), path, name,
                                      wxT( ".ses" ), wxT( "*.ses" ), frame(), wxFD_OPEN, false );

    if( !fullFileName.IsEmpty() )
        getEditFrame<PCB_EDIT_FRAME>()->ImportSpecctraSession( fullFileName );

    return 0;
}


int PCB_EDITOR_CONTROL::ExportSpecctraDSN( const TOOL_EVENT& aEvent )
{
    wxString    fullFileName;
    wxFileName  fn( frame()->GetBoard()->GetFileName() );

    fn.SetExt( SpecctraDsnFileExtension );

    fullFileName = EDA_FILE_SELECTOR( _( "Specctra DSN File" ), fn.GetPath(), fn.GetFullName(),
                                      SpecctraDsnFileExtension, SpecctraDsnFileWildcard(), 
                                      frame(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT, false );

    if( !fullFileName.IsEmpty() )
        getEditFrame<PCB_EDIT_FRAME>()->ExportSpecctraFile( fullFileName );
    
    return 0;
}


int PCB_EDITOR_CONTROL::GenerateFabFiles( const TOOL_EVENT& aEvent )
{
    wxCommandEvent dummy;
    
    if( aEvent.IsAction( &PCB_ACTIONS::generateGerbers ) )
        m_frame->ToPlotter( ID_GEN_PLOT_GERBER );
    else if( aEvent.IsAction( &PCB_ACTIONS::generateReportFile ) )
        m_frame->GenFootprintsReport( dummy );
    else if( aEvent.IsAction( &PCB_ACTIONS::generateD356File ) )
        m_frame->GenD356File( dummy );
    else if( aEvent.IsAction( &PCB_ACTIONS::generateBOM ) )
        m_frame->RecreateBOMFileFromBoard( dummy );
    else
        wxFAIL_MSG( "GenerateFabFiles(): unexpected request" );
    
    return 0;
}


int PCB_EDITOR_CONTROL::UpdatePCBFromSchematic( const TOOL_EVENT& aEvent )
{
    NETLIST netlist;

    if( m_frame->FetchNetlistFromSchematic( netlist, PCB_EDIT_FRAME::ANNOTATION_DIALOG ) )
    {
        DIALOG_UPDATE_PCB updateDialog( m_frame, &netlist );
        updateDialog.ShowModal();

        SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();

        if( !selectionTool->GetSelection().Empty() )
            m_toolMgr->InvokeTool( "pcbnew.InteractiveEdit" );
    }

    return 0;
}


int PCB_EDITOR_CONTROL::ToggleLayersManager( const TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->ToggleLayersManager();
    return 0;
}


int PCB_EDITOR_CONTROL::ToggleMicrowaveToolbar( const TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->ToggleMicrowaveToolbar();
    return 0;
}


int PCB_EDITOR_CONTROL::TogglePythonConsole( const TOOL_EVENT& aEvent )
{
#if defined( KICAD_SCRIPTING_WXPYTHON )
    m_frame->ScriptingConsoleEnableDisable();
#endif
    return 0;
}


int PCB_EDITOR_CONTROL::Find( const TOOL_EVENT& aEvent )
{
    DIALOG_FIND dlg( m_frame );
    dlg.ShowModal();
    return 0;
}


// Track & via size control
int PCB_EDITOR_CONTROL::TrackWidthInc( const TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>();
    int widthIndex = board->GetDesignSettings().GetTrackWidthIndex() + 1;

    if( widthIndex >= (int) board->GetDesignSettings().m_TrackWidthList.size() )
        widthIndex = board->GetDesignSettings().m_TrackWidthList.size() - 1;

    board->GetDesignSettings().SetTrackWidthIndex( widthIndex );
    board->GetDesignSettings().UseCustomTrackViaSize( false );

    m_toolMgr->RunAction( PCB_ACTIONS::trackViaSizeChanged, true );

    return 0;
}


int PCB_EDITOR_CONTROL::TrackWidthDec( const TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>();
    int widthIndex = board->GetDesignSettings().GetTrackWidthIndex() - 1;

    if( widthIndex < 0 )
        widthIndex = 0;

    board->GetDesignSettings().SetTrackWidthIndex( widthIndex );
    board->GetDesignSettings().UseCustomTrackViaSize( false );

    m_toolMgr->RunAction( PCB_ACTIONS::trackViaSizeChanged, true );

    return 0;
}


int PCB_EDITOR_CONTROL::ViaSizeInc( const TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>();
    int sizeIndex = board->GetDesignSettings().GetViaSizeIndex() + 1;

    if( sizeIndex >= (int) board->GetDesignSettings().m_ViasDimensionsList.size() )
        sizeIndex = board->GetDesignSettings().m_ViasDimensionsList.size() - 1;

    board->GetDesignSettings().SetViaSizeIndex( sizeIndex );
    board->GetDesignSettings().UseCustomTrackViaSize( false );

    m_toolMgr->RunAction( PCB_ACTIONS::trackViaSizeChanged, true );

    return 0;
}


int PCB_EDITOR_CONTROL::ViaSizeDec( const TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>();
    int sizeIndex = board->GetDesignSettings().GetViaSizeIndex() - 1;

    if( sizeIndex < 0 )
        sizeIndex = 0;

    board->GetDesignSettings().SetViaSizeIndex( sizeIndex );
    board->GetDesignSettings().UseCustomTrackViaSize( false );

    m_toolMgr->RunAction( PCB_ACTIONS::trackViaSizeChanged, true );

    return 0;
}


int PCB_EDITOR_CONTROL::PlaceModule( const TOOL_EVENT& aEvent )
{
    MODULE* module = aEvent.Parameter<MODULE*>();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    BOARD_COMMIT commit( m_frame );
    BOARD* board = getModel<BOARD>();

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    controls->ShowCursor( true );
    controls->SetSnapping( true );

    Activate();
    m_frame->SetToolID( ID_PCB_MODULE_BUTT, wxCURSOR_PENCIL, _( "Add footprint" ) );

    // Add all the drawable parts to preview
    VECTOR2I cursorPos = controls->GetCursorPosition();

    if( module )
    {
        module->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
        m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, module );
    }

    bool reselect = false;

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        // This can be reset by some actions (e.g. Save Board), so ensure it stays set.
        m_frame->GetGalCanvas()->SetCurrentCursor( wxCURSOR_PENCIL );
        cursorPos = controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( reselect && module )
            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, module );

        if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
        {
            if( module )
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
                commit.Revert();
                module = NULL;
            }
            else    // let's have another chance placing a module
                break;

            if( evt->IsActivate() )  // now finish unconditionally
                break;
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !module )
            {
                // Pick the module to be placed
                module = m_frame->SelectFootprintFromLibTree();

                if( module == NULL )
                    continue;

                module->SetLink( 0 );

                module->SetFlags( IS_NEW ); // whatever
                module->SetTimeStamp( GetNewTimeStamp() );

                // Set parent so that clearance can be loaded
                module->SetParent( board );

                // Put it on FRONT layer,
                // (Can be stored flipped if the lib is an archive built from a board)
                if( module->IsFlipped() )
                    module->Flip( module->GetPosition() );

                module->SetOrientation( 0 );
                module->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

                commit.Add( module );
                m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, module );
                controls->SetCursorPosition( cursorPos, false );
            }
            else
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
                commit.Push( _( "Place a module" ) );
                module = NULL;  // to indicate that there is no module that we currently modify
            }
        }

        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu(  selection()  );
        }

        else if( module && evt->IsMotion() )
        {
            module->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            selection().SetReferencePoint( cursorPos );
            getView()->Update( & selection()  );
        }

        else if( module && evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            // Calling 'Properties' action clears the selection, so we need to restore it
            reselect = true;
        }

        // Enable autopanning and cursor capture only when there is a module to be placed
        controls->SetAutoPan( !!module );
        controls->CaptureCursor( !!module );
    }

    m_frame->SetNoToolSelected();

    return 0;
}


int PCB_EDITOR_CONTROL::ToggleLockSelected( const TOOL_EVENT& aEvent )
{
    return modifyLockSelected( TOGGLE );
}


int PCB_EDITOR_CONTROL::LockSelected( const TOOL_EVENT& aEvent )
{
    return modifyLockSelected( ON );
}


int PCB_EDITOR_CONTROL::UnlockSelected( const TOOL_EVENT& aEvent )
{
    return modifyLockSelected( OFF );
}


int PCB_EDITOR_CONTROL::modifyLockSelected( MODIFY_MODE aMode )
{
    SELECTION_TOOL*         selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const PCBNEW_SELECTION& selection = selTool->GetSelection();

    if( selection.Empty() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true );

    bool modified = false;

    for( auto i : selection )
    {
        auto item = static_cast<BOARD_ITEM*>( i );
        bool prevState = item->IsLocked();

        switch( aMode )
        {
            case ON:
                item->SetLocked( true );
                break;

            case OFF:
                item->SetLocked( false );
                break;

            case TOGGLE:
                item->SetLocked( !prevState );
                break;
        }

        // Check if we really modified an item
        if( !modified && prevState != item->IsLocked() )
            modified = true;
    }

    if( modified )
    {
        m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
        m_frame->OnModify();
    }

    return 0;
}


int PCB_EDITOR_CONTROL::PlaceTarget( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = getView();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    BOARD* board = getModel<BOARD>();
    PCB_TARGET* target = new PCB_TARGET( board );

    // Init the new item attributes
    target->SetLayer( Edge_Cuts );
    target->SetWidth( board->GetDesignSettings().GetLineThickness( Edge_Cuts ) );
    target->SetSize( Millimeter2iu( 5 ) );
    VECTOR2I cursorPos = controls->GetCursorPosition();
    target->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( view );
    preview.Add( target );
    view->Add( &preview );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    controls->SetSnapping( true );

    Activate();
    m_frame->SetToolID( ID_PCB_TARGET_BUTT, wxCURSOR_PENCIL, _( "Add layer alignment target" ) );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        // This can be reset by some actions (e.g. Save Board), so ensure it stays set.
        m_frame->GetGalCanvas()->SetCurrentCursor( wxCURSOR_PENCIL );
        cursorPos = controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
            break;

        else if( evt->IsAction( &PCB_ACTIONS::incWidth ) )
        {
            target->SetWidth( target->GetWidth() + WIDTH_STEP );
            view->Update( &preview );
        }

        else if( evt->IsAction( &PCB_ACTIONS::decWidth ) )
        {
            int width = target->GetWidth();

            if( width > WIDTH_STEP )
            {
                target->SetWidth( width - WIDTH_STEP );
                view->Update( &preview );
            }
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            assert( target->GetSize() > 0 );
            assert( target->GetWidth() > 0 );

            BOARD_COMMIT commit( m_frame );
            commit.Add( target );
            commit.Push( _( "Place a layer alignment target" ) );

            preview.Remove( target );

            // Create next PCB_TARGET
            target = new PCB_TARGET( *target );
            preview.Add( target );
        }

        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }

        else if( evt->IsMotion() )
        {
            target->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            view->Update( &preview );
        }
    }

    delete target;

    controls->SetSnapping( false );
    view->Remove( &preview );

    m_frame->SetNoToolSelected();

    return 0;
}


static bool mergeZones( BOARD_COMMIT& aCommit, std::vector<ZONE_CONTAINER *>& aOriginZones,
        std::vector<ZONE_CONTAINER *>& aMergedZones )
{
    for( unsigned int i = 1; i < aOriginZones.size(); i++ )
    {
        aOriginZones[0]->Outline()->BooleanAdd( *aOriginZones[i]->Outline(),
                                                SHAPE_POLY_SET::PM_FAST );
    }

    aOriginZones[0]->Outline()->Simplify( SHAPE_POLY_SET::PM_FAST );

    // We should have one polygon with hole
    // We can have 2 polygons with hole, if the 2 initial polygons have only one common corner
    // and therefore cannot be merged (they are dectected as intersecting)
    // but we should never have more than 2 polys
    if( aOriginZones[0]->Outline()->OutlineCount() > 1 )
    {
        wxLogMessage( "BOARD::CombineAreas error: more than 2 polys after merging" );
        return false;
    }

    for( unsigned int i = 1; i < aOriginZones.size(); i++ )
    {
        aCommit.Remove( aOriginZones[i] );
    }

    aCommit.Modify( aOriginZones[0] );
    aMergedZones.push_back( aOriginZones[0] );

    aOriginZones[0]->SetLocalFlags( 1 );
    aOriginZones[0]->Hatch();
    aOriginZones[0]->CacheTriangulation();

    return true;
}


int PCB_EDITOR_CONTROL::ZoneMerge( const TOOL_EVENT& aEvent )
{
    const PCBNEW_SELECTION& selection = m_toolMgr->GetTool<SELECTION_TOOL>()->GetSelection();
    BOARD*                  board = getModel<BOARD>();
    BOARD_COMMIT            commit( m_frame );

    if( selection.Size() < 2 )
        return 0;

    int netcode = -1;

    ZONE_CONTAINER* firstZone = nullptr;
    std::vector<ZONE_CONTAINER*> toMerge, merged;

    for( auto item : selection )
    {
        auto curr_area = dynamic_cast<ZONE_CONTAINER*>( item );

        if( !curr_area )
            continue;

        if( !firstZone )
            firstZone = curr_area;

        netcode = curr_area->GetNetCode();

        if( firstZone->GetNetCode() != netcode )
            continue;

        if( curr_area->GetPriority() != firstZone->GetPriority() )
            continue;

        if( curr_area->GetIsKeepout() != firstZone->GetIsKeepout() )
            continue;

        if( curr_area->GetLayer() != firstZone->GetLayer() )
            continue;

        if( !board->TestAreaIntersection( curr_area, firstZone ) )
            continue;

        toMerge.push_back( curr_area );
    }

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    if( mergeZones( commit, toMerge, merged ) )
    {
        commit.Push( _( "Merge zones" ) );

        for( auto item : merged )
            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, item );
    }

    return 0;
}


int PCB_EDITOR_CONTROL::ZoneDuplicate( const TOOL_EVENT& aEvent )
{
    auto selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const auto& selection = selTool->GetSelection();

    // because this pops up the zone editor, it would be confusing to handle multiple zones,
    // so just handle single selections containing exactly one zone
    if( selection.Size() != 1 )
        return 0;

    auto oldZone = dyn_cast<ZONE_CONTAINER*>( selection[0] );

    if( !oldZone )
        return 0;

    ZONE_SETTINGS zoneSettings;
    zoneSettings << *oldZone;
    int dialogResult;

    if( oldZone->GetIsKeepout() )
        dialogResult = InvokeKeepoutAreaEditor( m_frame, &zoneSettings );
    else if( oldZone->IsOnCopperLayer() )
        dialogResult = InvokeCopperZonesEditor( m_frame, &zoneSettings );
    else
        dialogResult = InvokeNonCopperZonesEditor( m_frame, &zoneSettings );

    if( dialogResult != wxID_OK )
        return 0;

    // duplicate the zone
    BOARD_COMMIT commit( m_frame );

    auto newZone = std::make_unique<ZONE_CONTAINER>( *oldZone );
    newZone->ClearSelected();
    newZone->UnFill();
    zoneSettings.ExportSetting( *newZone );

    // If the new zone is on the same layer(s) as the the initial zone,
    // offset it a bit so it can more easily be picked.
    if( oldZone->GetIsKeepout() && ( oldZone->GetLayerSet() == zoneSettings.m_Layers ) )
        newZone->Move( wxPoint( IU_PER_MM, IU_PER_MM ) );
    else if( !oldZone->GetIsKeepout() && ( oldZone->GetLayer() == zoneSettings.m_CurrentZone_Layer ) )
        newZone->Move( wxPoint( IU_PER_MM, IU_PER_MM ) );

    commit.Add( newZone.release() );
    commit.Push( _( "Duplicate zone" ) );

    return 0;
}


int PCB_EDITOR_CONTROL::CrossProbePcbToSch( const TOOL_EVENT& aEvent )
{
    // Don't get in an infinite loop PCB -> SCH -> PCB -> SCH -> ...
    if( m_probingSchToPcb )
    {
        m_probingSchToPcb = false;
        return 0;
    }

    SELECTION_TOOL*         selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const PCBNEW_SELECTION& selection = selTool->GetSelection();

    if( selection.Size() == 1 )
        m_frame->SendMessageToEESCHEMA( static_cast<BOARD_ITEM*>( selection.Front() ) );
    else
        m_frame->SendMessageToEESCHEMA( nullptr );

    return 0;
}


int PCB_EDITOR_CONTROL::CrossProbeSchToPcb( const TOOL_EVENT& aEvent )
{
    BOARD_ITEM* item = aEvent.Parameter<BOARD_ITEM*>();

    if( item )
    {
        m_probingSchToPcb = true;
        getView()->SetCenter( VECTOR2D( item->GetPosition() ) );
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

        // If it is a pad and the net highlighting tool is enabled, highlight the net
        if( item->Type() == PCB_PAD_T && m_frame->GetToolId() == ID_PCB_HIGHLIGHT_BUTT )
        {
            int net = static_cast<D_PAD*>( item )->GetNetCode();
            m_toolMgr->RunAction( PCB_ACTIONS::highlightNet, false, net );
        }
        else
        // Otherwise simply select the corresponding item
        {
            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, item );
            // Ensure the display is refreshed, because in some installs
            // the refresh is done only when the gal canvas has the focus, and
            // that is not the case when crossprobing from Eeschema:
            m_frame->GetGalCanvas()->Refresh();
        }
    }

    return 0;
}


bool PCB_EDITOR_CONTROL::DoSetDrillOrigin( KIGFX::VIEW* aView, PCB_BASE_FRAME* aFrame,
                                           BOARD_ITEM* originViewItem, const VECTOR2D& aPosition )
{
    aFrame->SetAuxOrigin( wxPoint( aPosition.x, aPosition.y ) );
    originViewItem->SetPosition( wxPoint( aPosition.x, aPosition.y ) );
    aView->MarkDirty();
    aFrame->OnModify();
    return true;
}


bool PCB_EDITOR_CONTROL::SetDrillOrigin( KIGFX::VIEW* aView, PCB_BASE_FRAME* aFrame,
                                         BOARD_ITEM* originViewItem, const VECTOR2D& aPosition )
{
    aFrame->SaveCopyInUndoList( originViewItem, UR_DRILLORIGIN );
    return DoSetDrillOrigin( aView, aFrame, originViewItem, aPosition );
}


int PCB_EDITOR_CONTROL::DrillOrigin( const TOOL_EVENT& aEvent )
{
    Activate();

    PCBNEW_PICKER_TOOL* picker = m_toolMgr->GetTool<PCBNEW_PICKER_TOOL>();
    assert( picker );

    m_frame->SetToolID( ID_PCB_PLACE_OFFSET_COORD_BUTT, wxCURSOR_HAND, _( "Adjust zero" ) );
    picker->SetClickHandler( std::bind( SetDrillOrigin, getView(), m_frame,
                                        m_placeOrigin.get(), _1 ) );
    picker->Activate();
    Wait();

    return 0;
}


/**
 * Look for a BOARD_CONNECTED_ITEM in a given spot and if one is found - it enables
 * highlight for its net.
 *
 * @param aToolMgr is the TOOL_MANAGER currently in use.
 * @param aPosition is the point where an item is expected (world coordinates).
 * @param aUseSelection is true if we should use the current selection to pick the netcode
 */
static bool highlightNet( TOOL_MANAGER* aToolMgr, const VECTOR2D& aPosition,
                          bool aUseSelection = false )
{
    auto render = aToolMgr->GetView()->GetPainter()->GetSettings();
    auto frame = static_cast<PCB_EDIT_FRAME*>( aToolMgr->GetEditFrame() );

    BOARD* board = static_cast<BOARD*>( aToolMgr->GetModel() );

    int net = -1;
    bool enableHighlight = false;

    if( aUseSelection )
    {
        auto selectionTool = aToolMgr->GetTool<SELECTION_TOOL>();

        const PCBNEW_SELECTION& selection = selectionTool->GetSelection();

        for( auto item : selection )
        {
            if( BOARD_CONNECTED_ITEM::ClassOf( item ) )
            {
                auto ci = static_cast<BOARD_CONNECTED_ITEM*>( item );

                int item_net = ci->GetNetCode();

                if( net < 0 )
                {
                    net = item_net;
                }
                else if( net != item_net )
                {
                    // more than one net selected: do nothing
                    return 0;
                }
            }
        }

        enableHighlight = ( net >= 0 && net != render->GetHighlightNetCode() );
    }

    // If we didn't get a net to highlight from the selection, use the cursor
    if( net < 0 )
    {
        auto guide = frame->GetCollectorsGuide();
        GENERAL_COLLECTOR collector;

        // Find a connected item for which we are going to highlight a net
        collector.Collect( board, GENERAL_COLLECTOR::PadsOrTracks, (wxPoint) aPosition, guide );

        if( collector.GetCount() == 0 )
            collector.Collect( board, GENERAL_COLLECTOR::Zones, (wxPoint) aPosition, guide );

        // Clear the previous highlight
        frame->SendMessageToEESCHEMA( nullptr );

        for( int i = 0; i < collector.GetCount(); i++ )
        {
            if( ( collector[i]->GetLayerSet() & LSET::AllCuMask() ).none() )
                collector.Remove( i );

            if( collector[i]->Type() == PCB_PAD_T )
            {
                frame->SendMessageToEESCHEMA( static_cast<BOARD_CONNECTED_ITEM*>( collector[i] ) );
                break;
            }
        }

        enableHighlight = ( collector.GetCount() > 0 );

        // Obtain net code for the clicked item
        if( enableHighlight )
            net = static_cast<BOARD_CONNECTED_ITEM*>( collector[0] )->GetNetCode();
    }

    // Toggle highlight when the same net was picked
    if( net > 0 && net == render->GetHighlightNetCode() )
        enableHighlight = !render->IsHighlightEnabled();

    if( enableHighlight != render->IsHighlightEnabled() || net != render->GetHighlightNetCode() )
    {
        render->SetHighlight( enableHighlight, net );
        aToolMgr->GetView()->UpdateAllLayersColor();
    }

    // Store the highlighted netcode in the current board (for dialogs for instance)
    if( enableHighlight && net >= 0 )
    {
        board->SetHighLightNet( net );

        NETINFO_ITEM* netinfo = board->FindNet( net );

        if( netinfo )
        {
            MSG_PANEL_ITEMS items;
            netinfo->GetMsgPanelInfo( frame->GetUserUnits(), items );
            frame->SetMsgPanel( items );
            frame->SendCrossProbeNetName( netinfo->GetNetname() );
        }
    }
    else
    {
        board->ResetHighLight();
        frame->SetMsgPanel( board );
        frame->SendCrossProbeNetName( "" );
    }

    return true;
}


int PCB_EDITOR_CONTROL::HighlightNet( const TOOL_EVENT& aEvent )
{
    int netcode = aEvent.Parameter<intptr_t>();

    if( netcode > 0 )
    {
        KIGFX::RENDER_SETTINGS* render = m_toolMgr->GetView()->GetPainter()->GetSettings();
        render->SetHighlight( true, netcode );
        m_toolMgr->GetView()->UpdateAllLayersColor();
    }
    else
    {
        // No net code specified, pick the net code belonging to the item under the cursor
        highlightNet( m_toolMgr, getViewControls()->GetMousePosition() );
    }

    return 0;
}


int PCB_EDITOR_CONTROL::ClearHighlight( const TOOL_EVENT& aEvent )
{
    auto frame = static_cast<PCB_EDIT_FRAME*>( m_toolMgr->GetEditFrame() );
    auto board = static_cast<BOARD*>( m_toolMgr->GetModel() );
    KIGFX::RENDER_SETTINGS* render = m_toolMgr->GetView()->GetPainter()->GetSettings();

    board->ResetHighLight();
    render->SetHighlight( false );
    m_toolMgr->GetView()->UpdateAllLayersColor();
    frame->SetMsgPanel( board );
    frame->SendCrossProbeNetName( "" );
    return 0;
}


int PCB_EDITOR_CONTROL::HighlightNetCursor( const TOOL_EVENT& aEvent )
{
    // If the keyboard hotkey was triggered, the behavior is as follows:
    // If we are already in the highlight tool, behave the same as a left click.
    // If we are not, highlight the net of the selected item(s), or if there is
    // no selection, then behave like a Ctrl+Left Click.
    if( aEvent.IsAction( &PCB_ACTIONS::highlightNetSelection ) )
    {
        bool use_selection = ( m_frame->GetToolId() != ID_PCB_HIGHLIGHT_BUTT );
        highlightNet( m_toolMgr, getViewControls()->GetMousePosition(),
                      use_selection );
    }

    Activate();

    PCBNEW_PICKER_TOOL* picker = m_toolMgr->GetTool<PCBNEW_PICKER_TOOL>();
    assert( picker );

    m_frame->SetToolID( ID_PCB_HIGHLIGHT_BUTT, wxCURSOR_HAND, _( "Highlight net" ) );
    picker->SetClickHandler( std::bind( highlightNet, m_toolMgr, _1, false ) );
    picker->SetLayerSet( LSET::AllCuMask() );
    picker->Activate();
    Wait();

    return 0;
}


static bool showLocalRatsnest( TOOL_MANAGER* aToolMgr, BOARD* aBoard, const VECTOR2D& aPosition )
{
    auto selectionTool = aToolMgr->GetTool<SELECTION_TOOL>();

    aToolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    aToolMgr->RunAction( PCB_ACTIONS::selectionCursor, true, EDIT_TOOL::PadFilter );
    PCBNEW_SELECTION& selection = selectionTool->GetSelection();

    if( selection.Empty() )
    {
        aToolMgr->RunAction( PCB_ACTIONS::selectionCursor, true, EDIT_TOOL::FootprintFilter );
        selection = selectionTool->GetSelection();
    }

    if( selection.Empty() )
    {
        // Clear the previous local ratsnest if we click off all items
        for( auto mod : aBoard->Modules() )
        {
            for( auto pad : mod->Pads() )
                pad->SetLocalRatsnestVisible( aBoard->IsElementVisible( LAYER_RATSNEST ) );
        }
    }
    else
    {
        for( auto item : selection )
        {
            if( auto pad = dyn_cast<D_PAD*>(item) )
            {
                pad->SetLocalRatsnestVisible( !pad->GetLocalRatsnestVisible() );
            }
            else if( auto mod = dyn_cast<MODULE*>(item) )
            {
                bool enable = !( *( mod->Pads().begin() ) )->GetLocalRatsnestVisible();

                for( auto modpad : mod->Pads() )
                {
                    modpad->SetLocalRatsnestVisible( enable );
                }
            }
        }
    }

    aToolMgr->GetView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );

    return true;
}


int PCB_EDITOR_CONTROL::LocalRatsnestTool( const TOOL_EVENT& aEvent )
{
    Activate();

    auto picker = m_toolMgr->GetTool<PCBNEW_PICKER_TOOL>();
    auto board = getModel<BOARD>();
    wxASSERT( picker );
    wxASSERT( board );

    m_frame->SetToolID( ID_LOCAL_RATSNEST_BUTT, wxCURSOR_PENCIL,
                        _( "Pick Components for Local Ratsnest" ) );
    picker->SetClickHandler( std::bind( showLocalRatsnest, m_toolMgr, board, _1 ) );
    picker->SetFinalizeHandler( [ board ]( int aCondition ){
        auto vis = board->IsElementVisible( LAYER_RATSNEST );

        if( aCondition != PCBNEW_PICKER_TOOL::END_ACTIVATE )
        {
            for( auto mod : board->Modules() )
                for( auto pad : mod->Pads() )
                    pad->SetLocalRatsnestVisible( vis );
        }
        } );

    picker->Activate();
    Wait();

    return 0;
}


int PCB_EDITOR_CONTROL::UpdateSelectionRatsnest( const TOOL_EVENT& aEvent )
{
    auto selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    auto& selection = selectionTool->GetSelection();
    auto connectivity = getModel<BOARD>()->GetConnectivity();

    if( selection.Empty() )
    {
        connectivity->ClearDynamicRatsnest();
    }
    else if( m_slowRatsnest )
    {
        // Compute ratsnest only when user stops dragging for a moment
        connectivity->HideDynamicRatsnest();
        m_ratsnestTimer.Start( 20 );
    }
    else
    {
        // Check how much time doest it take to calculate ratsnest
        PROF_COUNTER counter;
        calculateSelectionRatsnest();
        counter.Stop();

        // If it is too slow, then switch to 'slow ratsnest' mode when
        // ratsnest is calculated when user stops dragging items for a moment
        if( counter.msecs() > 25 )
        {
            m_slowRatsnest = true;
            connectivity->HideDynamicRatsnest();
        }
    }

    return 0;
}


int PCB_EDITOR_CONTROL::HideDynamicRatsnest( const TOOL_EVENT& aEvent )
{
    getModel<BOARD>()->GetConnectivity()->HideDynamicRatsnest();
    m_slowRatsnest = false;
    return 0;
}


void PCB_EDITOR_CONTROL::ratsnestTimer( wxTimerEvent& aEvent )
{
    m_ratsnestTimer.Stop();
    calculateSelectionRatsnest();
    static_cast<PCB_DRAW_PANEL_GAL*>( m_frame->GetGalCanvas() )->RedrawRatsnest();
    m_frame->GetGalCanvas()->Refresh();
}


void PCB_EDITOR_CONTROL::calculateSelectionRatsnest()
{
    auto selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    auto& selection = selectionTool->GetSelection();
    auto connectivity = board()->GetConnectivity();

    std::vector<BOARD_ITEM*> items;
    items.reserve( selection.Size() );

    for( auto item : selection )
    {
        auto board_item = static_cast<BOARD_CONNECTED_ITEM*>( item );

        if( board_item->Type() != PCB_MODULE_T && board_item->GetLocalRatsnestVisible() )
        {
            items.push_back( board_item );
        }
        else if( board_item->Type() == PCB_MODULE_T )
        {
            for( auto pad : static_cast<MODULE*>( item )->Pads() )
            {
                if( pad->GetLocalRatsnestVisible() )
                    items.push_back( pad );
            }
        }
    }

    connectivity->ComputeDynamicRatsnest( items );
}


int PCB_EDITOR_CONTROL::FlipPcbView( const TOOL_EVENT& aEvent )
{
    view()->SetMirror( !view()->IsMirroredX(), false );
    view()->RecacheAllItems();
    frame()->Refresh();
    return 0;
}


void PCB_EDITOR_CONTROL::setTransitions()
{
    Go( &PCB_EDITOR_CONTROL::New,                    ACTIONS::doNew.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::Open,                   ACTIONS::open.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::Save,                   ACTIONS::save.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::SaveAs,                 ACTIONS::saveAs.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::SaveCopyAs,             ACTIONS::saveCopyAs.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::PageSettings,           ACTIONS::pageSettings.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::Plot,                   ACTIONS::plot.MakeEvent() );

    Go( &PCB_EDITOR_CONTROL::BoardSetup,             PCB_ACTIONS::boardSetup.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ImportNetlist,          PCB_ACTIONS::importNetlist.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ImportSpecctraSession,  PCB_ACTIONS::importSpecctraSession.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ExportSpecctraDSN,      PCB_ACTIONS::exportSpecctraDSN.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::GenerateDrillFiles,     PCB_ACTIONS::generateDrillFiles.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::GenerateFabFiles,       PCB_ACTIONS::generateGerbers.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::GeneratePosFile,        PCB_ACTIONS::generatePosFile.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::GenerateFabFiles,       PCB_ACTIONS::generateReportFile.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::GenerateFabFiles,       PCB_ACTIONS::generateD356File.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::GenerateFabFiles,       PCB_ACTIONS::generateBOM.MakeEvent() );

    Go( &PCB_EDITOR_CONTROL::Find,                   ACTIONS::find.MakeEvent() );

    // Track & via size control
    Go( &PCB_EDITOR_CONTROL::TrackWidthInc,          PCB_ACTIONS::trackWidthInc.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::TrackWidthDec,          PCB_ACTIONS::trackWidthDec.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ViaSizeInc,             PCB_ACTIONS::viaSizeInc.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ViaSizeDec,             PCB_ACTIONS::viaSizeDec.MakeEvent() );

    // Zone actions
    Go( &PCB_EDITOR_CONTROL::ZoneMerge,              PCB_ACTIONS::zoneMerge.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ZoneDuplicate,          PCB_ACTIONS::zoneDuplicate.MakeEvent() );

    // Placing tools
    Go( &PCB_EDITOR_CONTROL::PlaceTarget,            PCB_ACTIONS::placeTarget.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::PlaceModule,            PCB_ACTIONS::placeModule.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::DrillOrigin,            PCB_ACTIONS::drillOrigin.MakeEvent() );

    // Other
    Go( &PCB_EDITOR_CONTROL::ToggleLockSelected,     PCB_ACTIONS::toggleLock.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::LockSelected,           PCB_ACTIONS::lock.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::UnlockSelected,         PCB_ACTIONS::unlock.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::CrossProbePcbToSch,     EVENTS::SelectedEvent );
    Go( &PCB_EDITOR_CONTROL::CrossProbePcbToSch,     EVENTS::UnselectedEvent );
    Go( &PCB_EDITOR_CONTROL::CrossProbePcbToSch,     EVENTS::ClearedEvent );
    Go( &PCB_EDITOR_CONTROL::CrossProbeSchToPcb,     PCB_ACTIONS::crossProbeSchToPcb.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::HighlightNet,           PCB_ACTIONS::highlightNet.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ClearHighlight,         PCB_ACTIONS::clearHighlight.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::HighlightNetCursor,     PCB_ACTIONS::highlightNetTool.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::HighlightNetCursor,     PCB_ACTIONS::highlightNetSelection.MakeEvent() );

    Go( &PCB_EDITOR_CONTROL::LocalRatsnestTool,      PCB_ACTIONS::localRatsnestTool.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::HideDynamicRatsnest,    PCB_ACTIONS::hideDynamicRatsnest.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::UpdateSelectionRatsnest,PCB_ACTIONS::updateLocalRatsnest.MakeEvent() );

    Go( &PCB_EDITOR_CONTROL::ListNets,               PCB_ACTIONS::listNets.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::UpdatePCBFromSchematic, ACTIONS::updatePcbFromSchematic.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ToggleLayersManager,    PCB_ACTIONS::showLayersManager.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ToggleMicrowaveToolbar, PCB_ACTIONS::showMicrowaveToolbar.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::TogglePythonConsole,    PCB_ACTIONS::showPythonConsole.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::FlipPcbView,            PCB_ACTIONS::flipBoard.MakeEvent() );
}


const int PCB_EDITOR_CONTROL::WIDTH_STEP = 100000;