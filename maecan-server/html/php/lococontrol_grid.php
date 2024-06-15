<link rel="stylesheet" href="styles/lococontrol_grid.css?<?php echo time() ?>">
<link rel="stylesheet" type="text/css" href="styles/nexus7.slider.css?<?php echo time() ?>">
<div id="lococontrol">
    <div id="throttle_left" class="throttle">
		<div class="slider_container">
			<div class=" button speed" id="speed_left">0 km/h</div>
			<div class="slider">
				<input type="range" id="slider_left" data-orientation="vertical" min="0" max="1000" value="0" step="1">
					
			</div>
			<div class="button change_direction" id="change_direction_left">
				<div id="rev_left" style="display: none;">
					<div style="width: 0; height: 0; border-bottom: 2rem solid transparent; border-top: 2rem solid transparent; border-right: 2rem solid #2FA938; float: left;"></div>
					<div style="width: 0; height: 0; border-bottom: 2rem solid transparent; border-top: 2rem solid transparent; border-left: 2rem solid #adadad; float: right;"></div>
				</div>
				<div id="fwd_left" style="display: inline-block;">
					<div style="width: 0; height: 0; border-bottom: 2rem solid transparent; border-top: 2rem solid transparent; border-right: 2rem solid #adadad; float: left;"></div>
					<div style="width: 0; height: 0; border-bottom: 2rem solid transparent; border-top: 2rem solid transparent; border-left: 2rem solid #2fa938; float: right;"></div>
				</div>
			</div>
		</div>
        <div class="functions">
            <div class="button lokliste_toggle" id="mfx_find"></div>
			<div class="button lokliste" id="lokliste_left"><img id="icon_left" src="" class="preview_icon">Lokauswahl
				<div id="lokliste_container_left" class="dropdown">
				</div>
            </div>
            <div class="address" id="address_left" style="display: none;"></div>
            <div class="f_left">
                <div class="button function_left">F0</div>
                <div class="button function_left">F1</div>
                <div class="button function_left">F2</div>
                <div class="button function_left">F3</div>
                <div class="button function_left">F4</div>
                <div class="button function_left">F5</div>
                <div class="button function_left">F6</div>
                <div class="button function_left">F7</div>
            </div>
            <div class="f_left">
                <div class="button function_left">F8</div>
                <div class="button function_left">F9</div>
                <div class="button function_left">F10</div>
                <div class="button function_left">F11</div>
                <div class="button function_left">F12</div>
                <div class="button function_left">F13</div>
                <div class="button function_left">F14</div>
                <div class="button function_left">F15</div>
            </div>
        </div>
    </div>

    <div id="throttle_right" class="throttle">
		<div class="slider_container">
			<div class=" button speed" id="speed_right">0 km/h</div>
			<div class="slider">
				<input type="range" id="slider_right" data-orientation="vertical" min="0" max="1000" value="0" step="1">
				
			</div>
			<div class="button change_direction" id="change_direction_right">
				<div id="rev_right" style="display: none;">
					<div style="width: 0; height: 0; border-bottom: 2rem solid transparent; border-top: 2rem solid transparent; border-right: 2rem solid #2FA938; float: left;"></div>
					<div style="width: 0; height: 0; border-bottom: 2rem solid transparent; border-top: 2rem solid transparent; border-left: 2rem solid #adadad; float: right;"></div>
				</div>
				<div id="fwd_right" style="display: inline-block;">
					<div style="width: 0; height: 0; border-bottom: 2rem solid transparent; border-top: 2rem solid transparent; border-right: 2rem solid #adadad; float: left;"></div>
					<div style="width: 0; height: 0; border-bottom: 2rem solid transparent; border-top: 2rem solid transparent; border-left: 2rem solid #2fa938; float: right;"></div>
				</div>
			</div>
    	</div>
        <div class="functions">
            <div class="button lokliste_toggle" id="lokliste_toggle_right"></div>
			<div class="button lokliste" id="lokliste_right"><img id="icon_right" src="" class="preview_icon">Lokauswahl
			   	<div id="lokliste_container_right" class="dropdown">
			</div>
                <!--LOKLISTE-->
            </div>
            <div class="address" id="address_right" style="display:none;"></div>
            <div class="f_right">
                <div class="button function_right">F0</div>
                <div class="button function_right">F1</div>
                <div class="button function_right">F2</div>
                <div class="button function_right">F3</div>
                <div class="button function_right">F4</div>
                <div class="button function_right">F5</div>
                <div class="button function_right">F6</div>
                <div class="button function_right">F7</div>
            </div>
            <div class="f_right">
                <div class="button function_right">F8</div>
                <div class="button function_right">F9</div>
                <div class="button function_right">F10</div>
                <div class="button function_right">F11</div>
                <div class="button function_right">F12</div>
                <div class="button function_right">F13</div>
                <div class="button function_right">F14</div>
                <div class="button function_right">F15</div>
            </div>
		</div>
	</div>
</div>
<script type="text/javascript" src="js/jquery.min.js"></script>
<script type="text/javascript" src="js/rangeslider.min.js"></script>
<script type="text/javascript" src="js/main.js"></script>
<!--script type="text/javascript" src="js/websocket.js"></script-->
<script type="text/javascript" src="js/funcsymb.js"></script>

<script type="text/javascript">
/*
function GetFuncSymb(nr) {
    switch (nr) {
    case 8:     return "<svg width=\"36\" height=\"36\">" +     // Rangiergang
                "<polyline points=\"5,24 5.2,21.9 5.7,19.9 6.6,18 7.8,16.3 9.3,14.8 11,13.6 12.9,12.7 14.9,12.2 17,12 19.1,12.2 21.1,12.7 23,13.6 24.7,14.8 26.2,16.3 27.4,18 28.3,19.9 28.8,21.9 29,24\" stroke=\"black\" stroke-width=\"0\" fill=\"black\"/>" +
				"<circle r=\"3\" cx=\"11\" cy=\"24\" fill=\"black\" /><circle r=\"3\" cx=\"23\" cy=\"24\" fill=\"black\" /><circle r=\"3\" cx=\"29\" cy=\"20\" fill=\"black\" /></svg>";
	case 2:		return "<svg width=\"36\" height=\"36\">" +     // ABV
				"<polyline points=\"8,16 16,16 16,10 20,10 20,16 28,16 28,28 8,28\" stroke=\"black\" stroke-width=\"0\" fill=\"black\"/>" +
				"<polyline points=\"18,7 18.9,7 19.7,7.2 20.5,7.4 21.2,7.7 21.8,8.1 22.3,8.5 22.7,9 22.9,9.5 23,10 22.9,10.5 22.7,11 22.3,11.5 21.8,11.9 21.2,12.3 20.5,12.6 19.7,12.8 18.9,13 18,13 17.1,13 16.3,12.8 15.5,12.6 14.8,12.3 14.2,11.9 13.7,11.5 13.3,11 13.1,10.5 13,10 13.1,9.5 13.3,9 13.7,8.5 14.2,8.1 14.8,7.7 15.5,7.4 16.3,7.2 17.1,7\" stroke=\"black\" stroke-width=\"0\" fill=\"black\"/>" +
				"<text x=\"12\" y=\"25\" fill=\"white\" font-size=\"10\">kg</text></svg>";
    case 1: 	return "<svg width=\"36\" height=\"36\">" +    // Stirnbeleuchtung
				"<polyline points=\"15.5,22.3 14.8,21.8 14.2,21.2 13.7,20.5 13.3,19.7 13.1,18.9 13,18 13.1,17.1 13.3,16.3 13.7,15.5 14.2,14.8 14.8,14.2 15.5,13.7 16.3,13.3 17.1,13.1 18,13 18.9,13.1 19.7,13.3 20.5,13.7 21.2,14.2 21.8,14.8 22.3,15.5 22.7,16.3 22.9,17.1 23,18 22.9,18.9 22.7,19.7 22.3,20.5 21.8,21.2 21.2,21.8 20.5,22.3\" stroke=\"black\" stroke-width=\"0\" fill=\"black\"/>" +
				"<polyline points=\"15,23 21,23 21,30 18,32 15,30\" stroke=\"black\" stroke-width=\"0\" fill=\"black\"/>" +
				"<polyline points=\"10.2,22 5,25.5\" stroke=\"black\" stroke-width=\"2\" class=\"button_on\"/>"  +
				"<polyline points=\"9,18 3,18\" stroke=\"black\" stroke-width=\"2\" class=\"button_on\"/>"   +
				"<polyline points=\"10.2,13.5 5,10.5\" stroke=\"black\" stroke-width=\"2\" class=\"button_on\"/>" +
				"<polyline points=\"13.5,10.2 10.5,5\" stroke=\"black\" stroke-width=\"2\" class=\"button_on\"/>" +
				"<polyline points=\"18,9 18,3\" stroke=\"black\" stroke-width=\"2\" class=\"button_on\"/>"    +
				"<polyline points=\"22.5,10.2 25.5,5\" stroke=\"black\" stroke-width=\"2\" class=\"button_on\"/>" +
				"<polyline points=\"25.8,13.5 31,10.5\" stroke=\"black\" stroke-width=\"2\" class=\"button_on\"/>" +
				"<polyline points=\"27,18 33,18\" stroke=\"black\" stroke-width=\"2\" class=\"button_on\"/>"  +
				"<polyline points=\"25.8,22.5 31,25.5\" stroke=\"black\" stroke-width=\"2\" class=\"button_on\"/>" +
				"</svg>";
    case 7: 	return "<svg width=\"36\" height=\"36\">" +    // Rauch
				"<polyline points=\"20,18 30,18 30,22 28,22 28,32 22,32 22,22 20,22 20,18\" stroke=\"black\" stroke-width=\"0\" fill=\"black\"/>" +
				"<polyline points=\"25,12 28.5,12.9 30,15 28.5,17.1 25,18 21.5,17.1 20,15 21.5,12.9\" stroke=\"black\" stroke-width=\"0\" fill=\"black\" class=\"button_on smoke1\"/>" +
				"<polyline points=\"25,12 28.5,12.9 30,15 28.5,17.1 25,18 21.5,17.1 20,15 21.5,12.9\" stroke=\"black\" stroke-width=\"0\" fill=\"black\" class=\"button_on smoke2\"/></svg>";
	case 9:		return "<svg width=\"36\" height=\"36\">" +    // Telexkupplung
				"<polyline points=\"1,14 4,14 7,10 15,10 12,13 10,13 10,23 12,23 15,26 7,26 4,22 1,22 1,14\" stroke=\"black\" stroke-width=\"0\" fill=\"black\"/>" +
				"<polyline points=\"35,14 32,14 29,10 21,10 24,13 26,13 26,23 24,23 21,26 29,26 32,22 35,22 35,14\" stroke=\"black\" stroke-width=\"0\" fill=\"black\"/></svg>";
    case 5:
    case 23: 	return "<svg width=\"36\" height=\"36\">" +    // Geräusch
				"<polyline points=\"8,15 10,15 10,21 8,21\" stroke=\"black\" stroke-width=\"0\" fill=\"black\"/>" +
				"<polyline points=\"11,15 18,8 18,28 11,21\" stroke=\"black\" stroke-width=\"0\" fill=\"black\"/>" +
				"<polyline points=\"24.9,12.2 25.8,13.5 26.5,14.9 26.9,16.4 27,18 26.9,19.6 26.5,21.1 25.8,22.5 24.9,23.8\" stroke=\"black\" stroke-width=\"2\" fill=\"none\" class=\"button_on sound1\"/>" +
				"<polyline points=\"28,9.6 29.3,11.5 30.2,13.6 30.8,15.7 31,18 30.8,20.3 30.2,22.4 29.3,24.5 28,26.4\" stroke=\"black\" stroke-width=\"2\" fill=\"none\" class=\"button_on sound2\"/>" +
				"<polyline points=\"31,7.1 32.7,9.5 34,12.2 34.7,15 35,18 34.7,21 34,23.8 32.7,26.5 31,28.9\" stroke=\"black\" stroke-width=\"2\" fill=\"none\" class=\"button_on sound3\"/></svg>";
	case 10:    return "<svg width=\"36\" height=\"36\">" +    // Horn
				"<polyline points=\"1,16 3,16 3,17 10,17 14,16 16,15 19,12 19,24 16,21 14,20 10,19 3,19 3,20 1,20\" stroke=\"black\" stroke-width=\"0\" fill=\"black\"/>" +
				"<polyline points=\"24.9,12.2 25.8,13.5 26.5,14.9 26.9,16.4 27,18 26.9,19.6 26.5,21.1 25.8,22.5 24.9,23.8\" stroke=\"black\" stroke-width=\"2\" fill=\"none\" class=\"button_on sound1\"/>" +
				"<polyline points=\"28,9.6 29.3,11.5 30.2,13.6 30.8,15.7 31,18 30.8,20.3 30.2,22.4 29.3,24.5 28,26.4\" stroke=\"black\" stroke-width=\"2\" fill=\"none\" class=\"button_on sound2\"/>" +
				"<polyline points=\"31,7.1 32.7,9.5 34,12.2 34.7,15 35,18 34.7,21 34,23.8 32.7,26.5 31,28.9\" stroke=\"black\" stroke-width=\"2\" fill=\"none\" class=\"button_on sound3\"/></svg>";
    default:    return "<svg width=\"36\" height=\"36\">" +    // Rest
				"<text x=\"8\" y=\"24\" fill=\"black\" font-size=\"12\">F" + String(nr) + "</text>" + "</svg>";

    }
}
*/
/*
* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* <ixam97@ixam97.de> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in return.
* Maximilian Goldschmidt
* ----------------------------------------------------------------------------
* MäCAN-Server
* https://github.com/Ixam97/MaeCAN-Server/
* ----------------------------------------------------------------------------
*/

	var sides = ['left', 'right'];

	var lokliste_toggle = [];
	var lokliste_button = [];
	var lokliste_container = [];
	var lokliste_visible = [];

	var change_direction = [];

	var icon_img = [];
	var function_button = [];
	var speed = [];
	var slider = [];
	var slider_v = [];

	var sliding = [];
	var speedupdate = [];
	var old_speed = [];
	var direction = [];
	var fwd = [];
	var rev = [];

	var interval = [];
	var timer = [];

	//---LOKLISTE:
	var names = [];
	var uids = [];
	var icons = [];
	var vmaxes = [];

	var uid = [];
	var name_side = [];
	var tachomax = [];
	var icon = [];
	var fn_state = [];

	var locolist = [];
	let loco = [];

	for (var i = 0; i < sides.length; i++) {
		var side = sides[i];
		
		var mfx_find = document.getElementById('mfx_find');

		//lokliste_toggle[side] = document.getElementById(`lokliste_toggle_${side}`);
		lokliste_button[side] = document.getElementById(`lokliste_${side}`);
		lokliste_container[side] = document.getElementById(`lokliste_container_${side}`);
		lokliste_visible[side] = false;

		change_direction[side] = document.getElementById(`change_direction_${side}`);

		icon_img[side] = document.getElementById(`icon_${side}`);
		function_button[side] = document.getElementsByClassName(`function_${side}`);
		speed[side] = document.getElementById(`speed_${side}`);
		slider[side] = $(`#slider_${side}`);
		slider_v[side] = document.getElementById(`slider_${side}`);

		sliding[side] = false;
		speedupdate[side] = false;
		old_speed[side] = 0;
		direction[side] = 1;
		fwd[side] = document.getElementById(`fwd_${side}`);
		rev[side] = document.getElementById(`rev_${side}`);

		fn_state[side] = [];

		slider[side].rangeslider({polyfill: false});

		

		//loco[side].uid = 0;
	}

	slider['left'].on('input', function(){
		if (speedupdate['left']) {
			speedupdate['left'] = !speedupdate['left'];
		} else {
				if (!sliding['left']) {
					interval['left'] = setInterval(function(){
					setSpeed('left', slider_v['left'].value);
				}, 20),
				sliding['left'] = true;
			}
			clearTimeout(timer['left']);
			timer['left'] = setTimeout(function(){
			clearInterval(interval['left']);
				sliding['left'] = false;
			}, 500);
		}
	});

	slider['right'].on('input', function(){
		if (speedupdate['right']) {
			speedupdate['right'] = !speedupdate['right'];
		} else {
				if (!sliding['right']) {
					interval['right'] = setInterval(function(){
					setSpeed('right', slider_v['right'].value);
				}, 20),
				sliding['right'] = true;
			}
			clearTimeout(timer['right']);
			timer['right'] = setTimeout(function(){
			clearInterval(interval['right']);
				sliding['right'] = false;
			}, 500);
		}
	});

	//------------------FUNKTIONSTASTEN---------------------------//


	for (var j = 0; j < sides.length; j++) {
		var side = sides[j];
		for (var i = 0; i < function_button[side].length; i++) (function(i, side){
			function_button[side][i].onclick = function(){
                if (loco[side]) {		
				if (loco[side].uid >= 0x4000 || i <= 4) {
					setFn(loco[side].uid, i, !fn_state[side][i]);
				}
				if (loco[side].uid < 0x4000 && 4 < i && i<= 8) {
					setFn(loco[side].uid + 1, i - 4, !fn_state[side][i]);
				}
				if (loco[side].uid < 0x4000 && 8 < i && i <= 12) {
					setFn(loco[side].uid + 2, i - 8, !fn_state[side][i]);
				}
				if (loco[side].uid < 0x4000 && 12 < i && i <= 16) {
					setFn(loco[side].uid + 3, i - 12, !fn_state[side][i]);
				}
                }
				return false;
			};
		})(i, side);
	}

	function createDropdownPoint(name, icon, side) {
		let obj_text = document.createTextNode(name);
		let obj_icon = document.createElement('img');
		if (!icon) icon = "default.png";
		obj_icon.src = './loco_icons/' + icon;
		obj_icon.className = 'preview_icon';
		let obj = document.createElement('div');
		obj.clasName = 'locolist_dropdown_option';
		obj.appendChild(obj_icon);
		obj.appendChild(obj_text);
		lokliste_container[side].appendChild(obj);
		return obj;
	}

	function setLoco(side, _loco) {
		loco[side] = _loco;
//		loadFn(side);
		getDir(loco[side].uid);
//		getSpeed(loco[side].uid);
		if (!loco[side].icon) {
			icon_img[side].src = './loco_icons/default.png';
		} else {
			icon_img[side].src = './loco_icons/' + loco[side].icon;
		}
		lokliste_button[side].childNodes[1].textContent = loco[side].name;
		console.log('new selection with functions: ' + loco[side].functions);
		for (var i = 0; i < function_button[side].length; i++) {
			if ((i < loco[side].functions.length) && (loco[side].functions[i] > 0)) {
				function_button[side][i].style.visibility = "visible";
                function_button[side][i].innerHTML = GetFuncSymb(loco[side].functions[i]);
				getFn(loco[side].uid, i);
			} else {
				function_button[side][i].style.visibility = "hidden";
                function_button[side][i].innerHTML = GetFuncSymb(0);
			}
		}
	}

	for (var side in change_direction) (function(side){

		change_direction[side].onclick = function(){
            if (loco[side]) 
				parent.send(`toggleDir:${loco[side].uid}`);
			return false;
		}

		lokliste_button[side].onclick = function(){
			lokliste_container[side].innerHTML = "";
			for (let i = 0; i < locolist.length; i++) {
				let _loco = locolist[i];
				createDropdownPoint(locolist[i].name, locolist[i].icon, side).onclick = () => {
					setLoco(side, _loco);
				};
			}
			if (lokliste_visible[side]) {
				lokliste_visible[side] = false;
				hide(lokliste_container[side]);
				lokliste_button[side].className = 'button lokliste';
			} else {
				lokliste_visible[side] = true;
				show(lokliste_container[side]);
				lokliste_button[side].className = 'button button_dropdown lokliste';
			}
		}
	})(side);
	

	//--------------------------LOKLISTEN------------------------//



	//---------------------------INITIAL-------------------------//

//	ws.onopen = function(){
		//getLokliste(true);
		/*
		let locolist_request = new XMLHttpRequest();
		locolist_request.open('GET', './config/locolist.json', true);
		locolist_request.onload = function(){
			if (this.status == 200) {
				locolist = JSON.parse(this.responseText);
				setTimeout(() => setLoco('left', locolist[0]), 200);
				setTimeout(() => setLoco('right', locolist[1]), 200);
			}
		};
		locolist_request.send();
		*/
//	};

	//---------------------------FUNKTIONEN-----------------------------------//
/*
	function loadFn(side){
		var f = Math.min(function_button[side].length, loco[side].functions.length);
		for (var i = 0; i < f; i++) {
			if (loco[side].uid >= 0x4000 || i <= 4) {
				getFn(loco[side].uid, i);
			}
			if (loco[side].uid < 0x4000 && 4 < i && i <= 8) {
				getFn(loco[side].uid + 1, i - 4);
			}
			if (loco[side].uid < 0x4000 && 8 < i && i <= 12) {
				getFn(loco[side].uid + 2, i - 8);
			}
			if (loco[side].uid < 0x4000 && 12 < i && i <= 16) {
				getFn(loco[side].uid + 3, i - 12);
			}
		}
	}
*/

	function updateFn(value, side){
		var fn_num = value >> 8;
		var fn_value = value & 0x00ff;
		if(fn_value){
			function_button[side][fn_num].setAttribute('class', `button function_${side} button_active`);
		}else{
			function_button[side][fn_num].setAttribute('class', `button function_${side}`);
		}
        let myElements = function_button[side][fn_num].querySelectorAll("polyline");
        for (let i = 0; i < myElements.length; i++) {
            if (myElements[i].classList.contains('button_on')) {
                myElements[i].style.visibility = fn_value ? "visible" : "hidden";
            }
        }
		fn_state[side][fn_num] = fn_value;
		
	}

	function getFn(uid, value){
		parent.send(`getFn:${uid}:${value}`);
	}

	function setFn(uid, fn_num, fn_value){
		var value = (fn_num << 8) + fn_value;
		parent.send(`setFn:${uid}:${value}`);
	}

	function updateSpeed(side, value){
		speedupdate[side] = true;
		let tachomax = 100;
		let unit = "%";
		if(loco[side].tachomax) {
			tachomax = loco[side].tachomax;
			unit = "km/h";
		}
		speed[side].textContent = Math.ceil((value / 10) * (tachomax / 100)) + unit;
      	slider[side].val(value).change();
      	old_speed[side] = value;
	}

	function getSpeed(uid){
		parent.send(`getSpeed:${uid}`);
	}

	function setSpeed(side, value){
		if (loco[side] && (value != old_speed[side])) {
			parent.send(`setSpeed:${loco[side].uid}:${value}`);
		}
		old_speed[side] = value;
	}

	function updateDir(side, dir){
		if (dir == 1) {
			rev[side].style.display = 'none';
			fwd[side].style.display = 'inline-block';
			direction[side] = dir;
		} else if (dir == 2) {
			rev[side].style.display = 'inline-block';
			fwd[side].style.display = 'none';
			direction[side] = dir;
		} else {
			getDir(loco[side].uid);
		}
		getSpeed(loco[side].uid);
	}

	function getDir(uid){
		parent.send(`getDir:${uid}`);
	}

	function setDir(uid, dir){
		parent.send(`setDir:${uid}:${dir}`);
	}

	/*mfx_find.onclick = function() {
		send('mfxDiscovery');
		console.log('mfxDiscovery');
	}*/

	//------------------------------LISTENER---------------------------------//

//	ws.onmessage = function(dgram){
	function wsmessage(dgram){
		//console.log(`Recieved: ${dgram.data.toString()}.`);
		var msg = dgram.data.toString().split(':');
		var cmd = msg[0];
		var value = parseInt(msg[2]);
        var done = false;

		for (var i = 0; i < sides.length; i++) (function(side){

			if (loco[side]) {	
				if (cmd == 'updateFn') {
                    done = true;
					if ((msg[1] == loco[side].uid && loco[side].uid >= 0x4000) || (msg[1] == loco[side].uid && (value >> 8) <= 4 )) {
						updateFn(value, side);
					}
/*					if (loco[side].uid < 0x4000 && msg[1] == loco[side].uid + 1) {
						updateFn(value + 0x400, side);
					}
					if (loco[side].uid < 0x4000 && msg[1] == loco[side].uid + 2) {
						updateFn(value + 0x800, side);
					}
					if (loco[side].uid < 0x4000 && msg[1] == loco[side].uid + 3) {
						updateFn(value + 0xc00, side);
					}
*/				}

				if (cmd == 'updateSpeed') { 
                    done = true;
                    if (loco[side].uid == msg[1])
					   updateSpeed(side, value);
				}

				if (cmd == 'updateDir') {
                    done = true;
                    if (loco[side].uid == msg[1]) 
					   updateDir(side, msg[2]);
				}
			}
		})(sides[i]);
        return done;
	};

</script>