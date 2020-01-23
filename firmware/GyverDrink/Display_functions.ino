void DisplayRedraw(String label, byte hPosition, byte vPosition ){
                                                                  u8g2.clearBuffer();
                                                                  u8g2.setFont(u8g2_font_5x8_t_cyrillic);
                                                                  if (!srvMode) if (!workMode) {
                                                                                                u8g2.setCursor(31, 7); u8g2.print ("Ручной режим");
                                                                                                u8g2.setCursor(27, 30); u8g2.print ("Нажмите кнопку");
                                                                                                } 
                                                                                    else {
                                                                                          u8g2.setCursor(12, 7); u8g2.print ("Автоматический режим");
                                                                                          u8g2.setCursor(25, 30); u8g2.print ("Поставьте рюмку");
                                                                                          }
                                                                  if (srvMode) {u8g2.setCursor(35, 7);u8g2.print ("-=СЕРВИС=-");}
                                                                  u8g2.sendBuffer();
                                                                  
                                                                  u8g2.setFont(u8g2_font_unifont_t_cyrillic);  // крупный
                                                                  u8g2.setCursor(hPosition, vPosition);
                                                                  u8g2.print(label);   
                                                                  u8g2.sendBuffer();
                                                                } //конец void DisplayRedraw()