/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file lib_field.h
 */

#ifndef CLASS_LIBENTRY_FIELDS_H
#define CLASS_LIBENTRY_FIELDS_H

#include <eda_text.h>
#include <lib_draw_item.h>


class SCH_LEGACY_PLUGIN_CACHE;


/**
 * Field object used in symbol libraries.  At least MANDATORY_FIELDS are always present
 * in a ram resident library symbol.  All constructors must ensure this because
 * the component property editor assumes it.
 * <p>
 * A field is a string linked to a component.  Unlike purely graphical text, fields can
 * be used in netlist generation and other tools (BOM).
 *
 *  The first 4 fields have a special meaning:
 *
 *  0 = REFERENCE
 *  1 = VALUE
 *  2 = FOOTPRINT (default Footprint)
 *  3 = DATASHEET (user doc link)
 *
 *  others = free fields
 * </p>
 *
 * @see enum NumFieldType
 */
class LIB_FIELD : public LIB_ITEM, public EDA_TEXT
{
    int      m_id;           ///< @see enum NumFieldType
    wxString m_name;         ///< Name (not the field text value itself, that is .m_Text)

    /**
     * Print the field.
     * <p>
     * If \a aData not NULL, \a aData must point a wxString which is used instead of
     * the m_Text
     * </p>
     */
    void print( wxDC* aDC, const wxPoint& aOffset, void* aData,
                const TRANSFORM& aTransform ) override;

    /**
     * Calculate the new circle at \a aPosition when editing.
     *
     * @param aPosition - The position to edit the circle in drawing coordinates.
     */
    void CalcEdit( const wxPoint& aPosition ) override;

    friend class SCH_LEGACY_PLUGIN_CACHE;   // Required to access m_name.

public:

    LIB_FIELD( int idfield = 2 );

    LIB_FIELD( int aID, wxString& aName );

    LIB_FIELD( LIB_PART * aParent, int idfield = 2 );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~LIB_FIELD();

    wxString GetClass() const override
    {
        return wxT( "LIB_FIELD" );
    }

    wxString GetTypeName() override
    {
        return _( "Field" );
    }

    /**
     * Object constructor initialization helper.
     */
    void Init( int idfield );

    /**
     * Returns the field name.
     *
     * The first four field IDs are reserved and therefore always return their respective
     * names.  The user definable fields will return FieldN where N is the ID of the field
     * when the m_name member is empty.
     *
     * @param aTranslate True to return translated field name (default).  False to return
     *                   the english name (useful when the name is used as keyword in
     *                   netlists ...)
     * @return Name of the field.
     */
    wxString GetName( bool aTranslate = true ) const;

    /**
     * Set a user definable field name to \a aName.
     *
     * Reserved fields such as value and reference are not renamed.  If the field name is
     * changed, the field modified flag is set.  If the field is the child of a component,
     * the parent component's modified flag is also set.
     *
     * @param aName - User defined field name.
     */
    void SetName( const wxString& aName );

    int GetId() const { return m_id; }
    void SetId( int aId ) { m_id = aId; }

    int GetPenSize( ) const override;

    /**
     * Copy parameters of this field to another field. Pointers are not copied.
     *
     * @param aTarget Target field to copy values to.
     */
    void Copy( LIB_FIELD* aTarget ) const;

    /**
     * @return true if the field value is void (no text in this field)
     */
    bool IsVoid() const
    {
        return m_Text.IsEmpty();
    }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    const EDA_RECT GetBoundingBox() const override;

    void GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList ) override;

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;

    LIB_FIELD& operator=( const LIB_FIELD& field );

    /**
     * Return the text of a field.
     *
     * If the field is the reference field, the unit number is used to
     * create a pseudo reference text.  If the base reference field is U,
     * the string U?A will be returned for unit = 1.
     *
     * @todo This should be handled by the field object.
     *
     * @param unit - The package unit number.  Only effects reference field.
     * @return Field text.
     */
    wxString GetFullText( int unit = 1 ) const;

    COLOR4D GetDefaultColor() override;

    void BeginEdit( const wxPoint aStartPoint ) override;

    void Rotate() override;

    /**
     * Sets the field text to \a aText.
     *
     * This method does more than just set the set the field text.  There are special
     * cases when changing the text string alone is not enough.  If the field is the
     * value field, the parent component's name is changed as well.  If the field is
     * being moved, the name change must be delayed until the next redraw to prevent
     * drawing artifacts.
     *
     * @param aText - New text value.
     */
    void SetText( const wxString& aText ) override;

    void Offset( const wxPoint& aOffset ) override;

    bool Inside( EDA_RECT& aRect ) const override;

    void MoveTo( const wxPoint& aPosition ) override;

    wxPoint GetPosition() const override { return EDA_TEXT::GetTextPos(); }

    void MirrorHorizontal( const wxPoint& aCenter ) override;
    void MirrorVertical( const wxPoint& aCenter ) override;
    void Rotate( const wxPoint& aCenter, bool aRotateCCW = true ) override;

    void Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
               const TRANSFORM& aTransform ) override;

    int GetWidth() const override { return GetThickness(); }
    void SetWidth( int aWidth ) override { SetThickness( aWidth ); }

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

    BITMAP_DEF GetMenuImage() const override;

    EDA_ITEM* Clone() const override;

private:

    /**
     * @copydoc LIB_ITEM::compare()
     *
     * The field specific sort order is as follows:
     *
     *      - Field ID, REFERENCE, VALUE, etc.
     *      - Field string, case insensitive compare.
     *      - Field horizontal (X) position.
     *      - Field vertical (Y) position.
     *      - Field width.
     *      - Field height.
     */
    int compare( const LIB_ITEM& aOther ) const override;
};

typedef std::vector< LIB_FIELD > LIB_FIELDS;

#endif  //  CLASS_LIBENTRY_FIELDS_H