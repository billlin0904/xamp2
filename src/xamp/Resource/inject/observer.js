var wrapper = document.createElement("injectHtml");
wrapper.innerHTML = `
<script type="text/javascript">\
   new QWebChannel(qt.webChannelTransport, function (channel) {\
            observer = channel.objects.observer;\			
			console.warn('qwebchannel triggered');\
			window.setTimeout(function() {\
				let bar = document.querySelector('#progress-bar');\
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
</script>
	`;
document.body.appendChild(wrapper);

