"resource/ui/MapFinishedDialog.res"
{
    // Check HudLayout.res for the main panel's settings
    "CHudMapFinishedDialog"
    {
        "fieldName"     "CHudMapFinishedDialog"
        "xpos"          "c-110"
        "ypos"          "c-150"
        "wide"          "220"
        "tall"          "215"
        "TextFont"      "Default"
    }
    
    "Prev_Zone"
    {
        "ControlName"   "ImagePanel"
        "fieldName"     "Prev_Zone"
        "xpos"          "2"
        "ypos"          "4"
        "wide"          "8"
        "tall"          "8"
        "scaleImage"    "1"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "image"         "left_arrow_button_grey"
        "tooltiptext" "#MOM_MF_Left_Arrow"
    }
    
    "Current_Zone"//This is the label that shows what the current zone on the map finished panel
    {
        "ControlName" "Label"
        "fieldName"  "Current_Zone"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "0"
        "ypos"          "0"
        "wide"          "100"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "labelText"     "#MOM_MF_OverallStats"//This changes based on page number
        "textAlignment" "center"
        "dulltext"      "0"
        "brighttext"    "0"
        "textinsetx" "4"
        "auto_wide_tocontents" "1"
        "pin_to_sibling" "Prev_Zone"
        "pin_corner_to_sibling" "0"
        "pin_to_sibling_corner" "1"
    }
    
    "Next_Zone"
    {
        "ControlName"   "ImagePanel"
        "fieldName"     "Next_Zone"
        "xpos"          "0"
        "ypos"          "0"
        "wide"          "8"
        "tall"          "8"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "scaleImage"    "1"
        "image"         "right_arrow_button_grey"
        "pin_to_sibling" "Current_Zone"
        "pin_corner_to_sibling" "0"
        "pin_to_sibling_corner" "1"
        "tooltiptext" "#MOM_MF_Right_Arrow"
    }
    "Close_Panel"
    {
        "ControlName"   "ImagePanel"
        "fieldName"     "Close_Panel"
        "xpos"          "180"
        "ypos"          "2"
        "wide"          "32"
        "tall"          "32"
        "autoResize"    "0"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "scaleImage"    "1"
        "image"         "close_button"
    }
    "Repeat_Button"
    {
        "ControlName"   "ImagePanel"
        "fieldName"     "Repeat_Button"
        "xpos"          "0"
        "ypos"          "25"
        "wide"          "32"
        "tall"          "32"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "scaleImage"    "1"
        "image"         "loop_button_grey"
        "pin_to_sibling" "Close_Panel"
        "pin_to_sibling_corner" "2"
        "pin_corner_to_sibling" "0"
        "tooltiptext" "#MOM_MF_Restart_Map"
    }
    "Replay_Icon"
    {
        "ControlName"   "ImagePanel"
        "fieldName"     "Replay_Icon"
        "xpos"          "0"
        "ypos"          "5"
        "wide"          "32"
        "tall"          "32"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "scaleImage"    "1"
        "image"         "replay_icon_grey"
        "pin_to_sibling" "Repeat_Button"
        "pin_to_sibling_corner" "2"
        "pin_corner_to_sibling" "0"
        "tooltiptext" "#MOM_MF_PlayReplay"
    }
    
    "Zone_Overall_Time"// or "ZoneTime"
    {
        "ControlName" "Label"
        "fieldName"  "Zone_Overall_Time"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "6"
        "ypos"          "12"
        "wide"          "100"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "labelText"     "#MOM_MF_RunTime"//Possibly changing to #MOM_MF_Time_Zone
        "textAlignment" "west"
        "dulltext"      "0"
        "brighttext"    "0"
        "auto_wide_tocontents" "1"
    }
    
    "Zone_Enter_Time"
    {
        "ControlName" "Label"
        "fieldName"  "Zone_Enter_Time"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "0"
        "ypos"          "0"
        "wide"          "100"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "labelText"     "#MOM_MF_Zone_Enter"
        "textAlignment" "west"
        "dulltext"      "0"
        "brighttext"    "0"
        "pin_to_sibling"		"Zone_Overall_Time"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"2"
        "auto_wide_tocontents" "1"
    }
    
    "Zone_Strafes"
    {
        "ControlName" "Label"
        "fieldName"  "Zone_Strafes"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "0"
        "ypos"          "0"
        "wide"          "100"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "labelText"     "#MOM_MF_Strafes"
        "textAlignment" "west"
        "dulltext"      "0"
        "brighttext"    "0"
        "pin_to_sibling"		"Zone_Enter_Time"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"2"
        "auto_wide_tocontents" "1"
    }
    
    "Zone_Jumps"
    {
        "ControlName" "Label"
        "fieldName"  "Zone_Jumps"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "0"
        "ypos"          "0"
        "wide"          "100"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "labelText"     "#MOM_MF_Jumps"
        "textAlignment" "west"
        "dulltext"      "0"
        "brighttext"    "0"
        "pin_to_sibling"		"Zone_Strafes"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"2"
        "auto_wide_tocontents" "1"
    }
    
    "Zone_Vel_Enter"
    {
        "ControlName" "Label"
        "fieldName"  "Zone_Vel_Enter"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "0"
        "ypos"          "0"
        "wide"          "100"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "labelText"     "#MOM_MF_Velocity_Enter"
        "textAlignment" "west"
        "dulltext"      "0"
        "brighttext"    "0"
        "pin_to_sibling"		"Zone_Jumps"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"2"
        "auto_wide_tocontents" "1"
    }
    
    "Zone_Vel_Exit"
    {
        "ControlName" "Label"
        "fieldName"  "Zone_Vel_Exit"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "0"
        "ypos"          "0"
        "wide"          "100"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "labelText"     "#MOM_MF_Velocity_Exit"
        "textAlignment" "west"
        "dulltext"      "0"
        "brighttext"    "0"
        "pin_to_sibling"		"Zone_Vel_Enter"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"2"
        "auto_wide_tocontents" "1"
    }
    
    "Zone_Vel_Avg"
    {
        "ControlName" "Label"
        "fieldName"  "Zone_Vel_Avg"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "0"
        "ypos"          "0"
        "wide"          "100"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "labelText"     "#MOM_MF_Velocity_Avg"
        "textAlignment" "west"
        "dulltext"      "0"
        "brighttext"    "0"
        "pin_to_sibling"		"Zone_Vel_Exit"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"2"
        "auto_wide_tocontents" "1"
    }
    
    "Zone_Vel_Max"
    {
        "ControlName" "Label"
        "fieldName"  "Zone_Vel_Max"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "0"
        "ypos"          "0"
        "wide"          "100"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "labelText"     "#MOM_MF_Velocity_Max"
        "textAlignment" "west"
        "dulltext"      "0"
        "brighttext"    "0"
        "pin_to_sibling"		"Zone_Vel_Avg"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"2"
        "auto_wide_tocontents" "1"
    }
    
    "Zone_Sync1"
    {
        "ControlName" "Label"
        "fieldName"  "Zone_Sync1"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "0"
        "ypos"          "0"
        "wide"          "100"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "labelText"     "#MOM_MF_Sync1"
        "textAlignment" "west"
        "dulltext"      "0"
        "brighttext"    "0"
        "pin_to_sibling"		"Zone_Vel_Max"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"2"
        "auto_wide_tocontents" "1"
    }
    
    "Zone_Sync2"
    {
        "ControlName" "Label"
        "fieldName"  "Zone_Sync2"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "0"
        "ypos"          "0"
        "wide"          "100"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "labelText"     "#MOM_MF_Sync2"
        "textAlignment" "west"
        "dulltext"      "0"
        "brighttext"    "0"
        "pin_to_sibling"		"Zone_Sync1"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"2"
        "auto_wide_tocontents" "1"
    }
    
    "Run_Save_Status"
    {
        "ControlName" "Label"
        "fieldName"  "Run_Save_Status"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "0"
        "ypos"          "6"
        "wide"          "100"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "labelText"     "#MOM_MF_RunNotSaved"
        "textAlignment" "west"
        "dulltext"      "0"
        "brighttext"    "0"
        "pin_to_sibling"		"Zone_Sync2"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"2"
        "auto_wide_tocontents" "1"
    }
    
    "Run_Upload_Status"
    {
        "ControlName" "Label"
        "fieldName"  "Run_Upload_Status"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "0"
        "ypos"          "0"
        "wide"          "120"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "labelText"     "#MOM_MF_RunNotUploaded"
        "textAlignment" "west"
        "dulltext"      "0"
        "brighttext"    "0"
        "pin_to_sibling"		"Run_Save_Status"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"2"
        "auto_wide_tocontents" "1"
    }

    "XP_Gain_Cos"
    {
        "ControlName" "Label"
        "fieldName"  "XP_Gain_Cos"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "0"
        "ypos"          "5"
        "wide"          "120"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "0"
        "enabled"       "1"
        "labelText"     "#MOM_MF_XPGainCos"
        "textAlignment" "west"
        "dulltext"      "0"
        "brighttext"    "0"
        "pin_to_sibling"		"Run_Upload_Status"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"2"
        "auto_wide_tocontents" "1"
    }

    "Cos_Level_Gain"
    {
        "ControlName" "Label"
        "fieldName"  "Cos_Level_Gain"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "3"
        "ypos"          "0"
        "wide"          "120"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "0"
        "enabled"       "1"
        "labelText"     "#MOM_MF_CosLvlGain"
        "textAlignment" "west"
        "dulltext"      "0"
        "brighttext"    "0"
        "pin_to_sibling"		"XP_Gain_Cos"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"1"
        "auto_wide_tocontents" "1"
    }

    "XP_Gain_Rank"
    {
        "ControlName" "Label"
        "fieldName"  "XP_Gain_Rank"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "0"
        "ypos"          "0"
        "wide"          "120"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "0"
        "enabled"       "1"
        "labelText"     "#MOM_MF_XPGainRank"
        "textAlignment" "west"
        "dulltext"      "0"
        "brighttext"    "0"
        "pin_to_sibling"		"XP_Gain_Cos"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"2"
        "auto_wide_tocontents" "1"
    }
    
    "Mouse_State"
    {
        "ControlName" "Label"
        "fieldName" "Mouse_State"
        "font"          "Default"//Set by "TextFont" 
        "xpos"          "6"
        "ypos"          "5"
        "wide"          "f0"
        "tall"          "10"//Set by font size
        "autoResize"    "1"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "labelText"     "#MOM_MF_MouseToggle"
        "textAlignment" "center"
        "dulltext"      "0"
        "brighttext"    "0"
        "proportionalToParent" "1"
        "pin_to_sibling" "XP_Gain_Rank"
        "pin_to_sibling_corner" "2"
        "pin_corner_to_sibling" "0"
        "auto_wide_tocontents" "0"
        "auto_tall_tocontents" "1"
    }
}