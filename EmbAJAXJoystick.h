/**
 * 
 * EmbAJAX - Simplistic framework for creating and handling displays and controls on a WebPage served by an Arduino (or other small device).
 * 
 * Copyright (C) 2018 Thomas Friedrichsmeier
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
**/
//#error The EmbAJAXJoystick class is a work in progress, and not expected to work, yet. Check back, later.


#ifndef EMBAJAXJOYSTICK_H
#define EMBAJAXJOYSTICK_H

#include <EmbAJAX.h>

const char EmbAJAXJoystick_SNAP_BACK[] = "if (!pressed) { x = width / 2; y = height / 2; }\n";
const char EmbAJAXJoystick_NO_SNAP_BACK[] = "";
const char EmbAJAXJoystick_FREE_POSITION[] = "";
const char EmbAJAXJoystick_POSITION_9_DIRECTIONS[] = "if (pressed) {\n"
                                                      "  if (x < width/3) x = width / 6;\n"
                                                      "  else if (x > (2*width/3)) x = 5*width / 6\n"
                                                      "  else x = width / 2;\n"
                                                      "\n"
                                                      "  if (y < height/3) y = height / 6;\n"
                                                      "  else if (y > (2*height/3)) y = 5*height / 6\n"
                                                      "  else y = height / 2;\n"
                                                      "}\n";

/** This class provides a basic joystick for directional control. WORK IN PROGRESS, THIS IS NOT EXPECTED TO WORK, YET. */
class EmbAJAXJoystick : public EmbAJAXElement {
public:
    /** C'tor.
     * @param width: Element width
     * @param height: Element height
     * @param active_timeout: Minimum timeout between two position change notifications (in milliseconds)
     * @param idle_timeout: Timeout after which the position other than center will be regarded as uncertain (e.g. due to unreliable network connection). @Note: While the position is off-center in the client, updates will be send at half this timeout.
     * @param position_adjust: Custom javascript that will be applied to "correct" the user supplied position, e.g. snapping it to certain allowed positions. @See e.g. EmbAJAXJoystick_POSITION_9_DIRECTIONS
     * @param snap_back: Custom javascript that will be applied to snap back the position on mouse release. @See EmbAJAXJoystick_SNAP_BACK */
    EmbAJAXJoystick(const char* id, int width, int height, int active_timeout=100, int idle_timeout=2000, const char* position_adjust=EmbAJAXJoystick_FREE_POSITION, const char* snap_back=EmbAJAXJoystick_SNAP_BACK) : EmbAJAXElement(id) {
        _width = width;
        _height = height;
        _active_timeout = active_timeout;
        _idle_timeout = idle_timeout;
        _position_adjust = position_adjust;
        _snap_back = snap_back;
    }
    void print() const override {
        char buf[12];

        EmbAJAXBase::_driver->printContent("<canvas");
        EmbAJAXBase::_driver->printAttribute("id", _id);
        EmbAJAXBase::_driver->printAttribute("width", _width);
        EmbAJAXBase::_driver->printAttribute("height", _height);
        EmbAJAXBase::_driver->printContent("style=\"cursor: all-scroll\"/>");  // style="border-radius:50%; background-color:grey; cursor: all-scroll"
        EmbAJAXBase::_driver->printContent(
           "<script>\n"
           "var elem = document.getElementById(");
        EmbAJAXBase::_driver->printAttribute("id", _id);
        EmbAJAXBase::_driver->printContent(
           ");\n"
           "elem.prototype.__defineSetter__('coords', function(value) {\n"
           "  var vals = value.split(',');\n"
           "  update(vals[0], vals[1]);\n"
           "});\n"
           "elem.last_server_update = Date.now();\n"
           "elem.sendState = function() {\n"
           "  var act_t = ");
        EmbAJAXBase::_driver->printContent(itoa(_active_timeout, buf, 10));
        EmbAJAXBase::_driver->printContent(
           ";\n"
           "  if (Date.now() - this.last_server_update < act_t) {\n"
           "    window.setTimeout(function() { this.sendState() }.bind(this), act_t);\n"
           "  }\n"
           "  doRequest(this.id, this.pressed + ',' + this.posx + ',' + this.posy);\n"
           "  this.last_server_update = Date.now();\n"
           "}\n"
           "\n"
           "elem.applypos = function(x, y, pressed) {\n"
           "  var width = this.width;\n"
           "  var height = this.height;\n"
           "  x = Math.round(((x - width / 2) * 2000) / width);\n"    // Scale values to +/-1000, independent of display size
           "  y = Math.round(((y - height / 2) * 2000) / height);\n"
           );
        EmbAJAXBase::_driver->printContent(_snap_back);
        EmbAJAXBase::_driver->printContent(_position_adjust);
        EmbAJAXBase::_driver->printContent(
           "  this.posx = x;\n"
           "  this.posy = y;\n"
           "}\n"
           "\n"
           "elem.update = function(x, y) {\n"
           "  var oldx = this.posx;\n"
           "  var oldy = this.posy;\n"
           "  this.applypos(x, y, this.pressed);\n"
           "  if (this.posx != oldx || this.posy != oldy) {\n"
           "    var ctx = this.getContext('2d');\n"
           "    ctx.clearRect(0, 0, this.width, this.height);\n"
           "    this.drawKnob(ctx, this.posx, this.posy);\n"
           "    this.sendState();\n"
           "  }\n"
           "}\n"
           "\n"
           // TODO: This should be customizable
           "elem.drawKnob = function(ctx, x, y) {\n"
           "  x = x * width / 2000 + width / 2;\n"
           "  y = y * height / 2000 + height / 2;\n"
           "  ctx.beginPath();\n"
           "  ctx.arc(x, y, 15, 0, 2 * Math.PI);\n"
           "  ctx.stroke();\n"
           "  ctx.fill();\n"
           "}\n"
           "\n"
           "elem.press = function(x, y) {\n"
           "  this.pressed = 1;\n"
           "  this.update(x, y);\n"
           "}\n"
           "\n"
           "elem.move = function(x, y) {\n"
           "  this.update(x, y);\n"
           "}\n"
           "\n"
           "elem.release = function(x, y) {\n"
           "  this.pressed = 0;\n"
           "  this.update();\n"
           "}\n"
           "\n"
           "elem.addEventListener('mousedown', function(event) { elem.press(event.offsetX, event.offsetY); }, false);\n"
           "elem.addEventListener('mousemove', function(event) { elem.move(event.offsetX, event.offsetY); }, false);\n"
           "elem.addEventListener('mouseup', function(event) { elem.release(event.offsetX, event.offsetY); }, false);\n"
           "elem.addEventListener('mouseleave', function(event) { elem.release(event.offsetX, event.offsetY); }, false);\n"
           "elem.addEventListener('touchstart', function(event) { elem.press(event.touches[0].offsetX, event.touches[0].offsetY); }, false);\n"
           "elem.addEventListener('touchmove', function(event) { elem.move(event.touches[0].offsetX, event.touches[0].offsetY); }, false);\n"
           "elem.addEventListener('touchend', function(event) { elem.release(event.touches[0].offsetX, event.touches[0].offsetY); }, false);\n"
           "</script>\n");
    }
    void updateFromDriverArg(const char* argname) {
        const int bufsize = 16;
        char buf[bufsize];
        _driver->getArg(argname, buf, bufsize);
        // format: "P,X,Y", each a number. Parsing from reverse
        int p = bufsize;
        while (--p >= 0) if (buf[p] == '\0') break;
        while (--p >= 0) if (buf[p] == ',') break;
        _cury = atoi(buf + p + 1);
        buf[p] = '\0';
        while (--p >= 0) if (buf[p] == ',') break;
        _curx = atoi(buf + p + 1);
        buf[p] = '\0';
        _pressed = atoi(buf);
        updateValueString();
    }
    int getX() const { return _curx; };
    int getY() const { return _cury; };
    void setPosition (int x, int y) {
        if (x != _curx || y != _cury) {
            _curx = x;
            _cury = y;
            setChanged();
            updateValueString();
        }
    }
    const char* valueProperty(uint8_t which = EmbAJAXBase::Value) const override {
        if (which == EmbAJAXBase::Value) return "coords";
        return EmbAJAXElement::valueProperty(which);
    }
    const char* value(uint8_t which = EmbAJAXBase::Value) const override {
        if (which == EmbAJAXBase::Value) return _value;
        return EmbAJAXElement::value(which);
    }
private:
    void updateValueString() {
        itoa(_curx, _value, 10);
        int p = 0;
        while (_value[p] != '\0') ++p;
        _value[p] = ',';
        itoa(_cury, _value + p + 1, 10);
    }
    char _value[16];
    int _width;
    int _height;
    int _active_timeout;
    int _idle_timeout;
    const char* _snap_back;
    const char* _position_adjust;
    int _curx, _cury;
    bool _pressed;
};

#endif