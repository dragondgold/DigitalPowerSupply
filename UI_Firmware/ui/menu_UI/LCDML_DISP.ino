// =====================================================================
//
// Output function
//
// =====================================================================

/* ******************************************************************** */
void LCDML_lcd_menu_display()
/* ******************************************************************** */
{
  uint8_t heigthFrame = 14;
    // For first test set font here
    u8g.setFont(_LCDML_u8g_font);
    u8g.setColorIndex(1); // Instructs the display to draw with a pixel on.;
    if(!drawTitle){
      if(LCDML.getLayer() == 0){
        defaultTitle.toUpperCase();
        currentTitle = defaultTitle;
      }else if(previousLayer < LCDML.getLayer()){
        currentTitle.toUpperCase();
        previousTitle = currentTitle;
        nextTitle.toUpperCase();
        currentTitle = nextTitle;
      }else if(previousLayer == LCDML.getLayer()){
        previousTitle.toUpperCase();
        currentTitle = previousTitle;
      }
    }
    
  if (LCDML_DISP_update()) {   
    


    // Init vars
    uint8_t n_max = (LCDML.getChilds() >= _LCDML_u8g_rows) ? ((_LCDML_u8g_rows > _LCDML_u8g_rows_max) ? _LCDML_u8g_rows : _LCDML_u8g_rows_max) : (LCDML.getChilds());
    
    // Set page
    drawing = true;
    u8g.firstPage();
  
    // Generate content
    do { 
        if(error){
          drawError();
        }
        else {
          u8g.setColorIndex(1); // Instructs the display to draw with a pixel on.
          u8g.drawBox(0,0,128,heigthFrame-1);
          u8g.drawFrame(0,heigthFrame,128,_LCDML_u8g_lcd_h -heigthFrame);
          u8g.setColorIndex(0); // Instructs the display to draw with a pixel on.
          u8g.drawStr( 1, heigthFrame-4,currentTitle.c_str() ); 
          u8g.setColorIndex(1); // Instructs the display to draw with a pixel on.
        
          // display rows and cursor
          for (uint8_t n = 0; n < n_max; n++)
          {
            // set cursor
            if (n == LCDML.getCursorPos()) {
              
              nextTitle  = LCDML.content[n];
              u8g.setColorIndex(1); // Instructs the display to draw with a pixel on.
              u8g.drawBox( 1,(((_LCDML_u8g_font_h-1) * (LCDML.getCursorPos()+ 2))-10),121,_LCDML_u8g_font_h);
              u8g.setColorIndex(0); // Instructs the display to draw with a pixel on.
              u8g.drawStr( 4, (_LCDML_u8g_font_h-1) * (LCDML.getCursorPos() + 2), LCDML.content[n]); 
              u8g.setColorIndex(1); // Instructs the display to draw with a pixel on.
            }else {
              //u8g.setFont(u8g_font_6x10); 
              u8g.drawStr( 4, (_LCDML_u8g_font_h-1) * (n + 2), LCDML.content[n]);
            }
          }
  
          // Set frame for scrollbar
          u8g.drawFrame(_LCDML_u8g_lcd_w - _LCDML_u8g_scrollbar_w, heigthFrame, _LCDML_u8g_scrollbar_w, _LCDML_u8g_lcd_h -heigthFrame);
        
          // Set scrollbar
          u8g.drawBox(_LCDML_u8g_lcd_w - (_LCDML_u8g_scrollbar_w-1), heigthFrame + 1 + (LCDML.getCursorPosAbs()*((_LCDML_u8g_lcd_h -heigthFrame)/(LCDML.getChilds()))),(_LCDML_u8g_scrollbar_w-2)  , (_LCDML_u8g_lcd_h -heigthFrame)/LCDML.getChilds());
  
          drawTitle = true;
        }
    } while ( u8g.nextPage() );
    drawing = false;
  }
  // Reinit some vars
  LCDML_DISP_update_end();  
}

// LCD clear
void LCDML_lcd_menu_clear()
{
  
}

