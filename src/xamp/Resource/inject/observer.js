window.onload = () => {
var sc = document.createElement("script");	
sc.innerHTML = `
'use strict';
new QWebChannel(qt.webChannelTransport, function (channel) {\
			var observer = channel.objects.observer;\			
			console.warn('qwebchannel triggered');\
			window.setInterval(function() {\
				var bar = document.querySelector('#progress-bar');\
				var isExistPlayerBar = document.querySelector('.ytmusic-player-bar.byline') !== null;\
				if (isExistPlayerBar == false) {\
					return;\
				}\
               	var msg = {\
               		title: document.querySelector('.ytmusic-player-bar.title').textContent,\
               		by: document.querySelector('.ytmusic-player-bar.byline').textContent,\
               		thumbnail: document.querySelector('ytmusic-player-bar .middle-controls img.image').getAttribute('src'),\
               		progress: parseInt(bar.getAttribute('value')),\
               		length: parseInt(bar.getAttribute('aria-valuemax')),\
               		isPlaying: document.querySelector('.play-pause-button.ytmusic-player-bar').getAttribute('title') == window.yt.msgs_.YTP_PAUSE\
               	};\
				observer.postMessage(JSON.stringify(msg))\
			}, 1000);\
        });\
		`;
document.head.appendChild(sc);
	};
