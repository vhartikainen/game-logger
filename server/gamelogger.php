<!DOCTYPE html>
<html>
<head>
	<meta charset="utf-8">
	<title>GameLogger — History</title>
	<link rel="stylesheet" type="text/css" href="massakuuri.css">
</head>
<body>

<!-- Gamelogger history begins -->


	<!-- jQuery core must load before its plugins (mousewheel/touchSwipe) and gamelogger.js. -->
	<script type="text/javascript" src="gamelogger/jquery-1.7.2.min.js"></script>
	<script type="text/javascript" src="gamelogger/jquery.mousewheel.min.js"></script>
	<script type="text/javascript" src="gamelogger/jquery.touchSwipe.min.js"></script>
	<script type="text/javascript" src="gamelogger/gamelogger.js"></script>
	<script type="text/javascript" src="gamelogger/gamelogger.history.js"></script>
	<script type="text/javascript" src="gamelogger/gamelogger.history.game.histogram.js"></script>
	<script type="text/javascript" src="gamelogger/gamelogger.history.timeline.js"></script>
	<script type="text/javascript">
		/*
			The following structure is used to define a gradient for the games. Each widget will use the 
			gradient if they can. The data field contains the definition of the gradient.
			
			The order field defines how the games are ordered before they are assigned a color. The 
			possibilities are:
				id			Game id as it is in the settings
				name		Alphabetical order of the game names
				played		The sum of the time the game was played by each
				walltime	The time the game was played by anyone			
				
			gamelogger.history.js is responsible for converting the gradient to actual colors for each game.
			Be sure to include the history before including this script.
			
			*/
		gamelogger.history.gradient = {
			
			data: [
				{pos: 0, color: "#ff96c7"},
				{pos: 0.2, color: "#ff5e00"},
				{pos: 0.4, color: "#ffcf23"},
				{pos: 0.6, color: "#8de820"},
				{pos: 0.8, color: "#125234"},
				{pos: 1, color: "#444444"}],
				
			order: "played"
		};
	</script>	
		
	<!-- 
		GameLogger history histogram widget
		
		The attribute data-order defines which calculation is used to order the games. 
		
		The contents of this div defines the prototype which is replicated for each game in the histogram.
		The replication also changes game-related values to specified locations. The directive is to add
		data-instance="<type>" for the elements that need to be changed. The <type> parameter has the following
		values and corresponding meanings:
			name		The contents of the element is replaced with the name of the game
			pic			Same as above, but the game image is added to within the element
			played		Same as above, but the contents is replaced with the time the game was played. 
						This is the sum of the times each player had the game on.
			walltime	Same as above, but the amount is now the time at least one of the players had
						the game on. For example, if 4 people all had the game on for one day, this value
						is 24 hours. No matter if one or two didn't have the game on for the entire day,
						as long as at least one had it on for the whole day.						
			width-played The CSS width of the element is set to the percentage the game was played with
						respect to the most played game. For example, if Game1 was played 5h and Game2 was
						played 2h, and Game1 was the most played game, the element with widthPlayed for
						Game2 is set to 2/5*100% = 40%.
			width-walltime
						Same as above but the calculations are done with walltime.			
			bg			CSS background of the image is set to: url("<picurl>") no-repeat;
			bg-color	CSS background-color is set to the color of the game
			color		CSS color is set to the color of the game
	-->	
	<!--
		gl-history-loading
			The contents of elements with  class are cleared when loading and analyzing game history
			is done.
		
		gl-history-loaded
			CSS visibility: visible is set when history has been loaded. 
	 -->
	<div class="gl-history-loading"><h1>Loading data</h1></div>

	<div class="window wide">
		<div class="windowtitle"><h2>Game History</h2></div>
		<div class="windowcontent padding">
			<div class="gl-history-game-histogram" data-order="played">
				
				<div class="gl-wrapper">
					<div class="game">
						<div class="game-name"><span data-instance="name"></span></div>
						<div data-instance="pic" class="game-pic"></div>
						<div data-instance="bg-color" class="game-bg-color"></div>
						<div style="clear: both;"><span>Total time of game played</span></div>
						<div data-instance="played" class="game-played"></div>
						<div style="float: left; display: block; height: 40px; width: 80%;"><div data-instance="width-played" class="game-width-played"><span></span></div></div>
						<!--
						<div>Longest session played</div>
						<div data-instance="walltime" class="game-walltime"></div>
						<div style="float: left; display: block; height: 40px; width: 240px;"><div data-instance="width-walltime" class="game-width-walltime"><span>QV</span></div></div>
						<div data-instance="bg" class="game-bg"></div>
						
						<div data-instance="color" class="game-color"></div>
						-->
						
					</div>
					<div class="clear"></div>
				</div>
			</div>
		</div>
	</div>
	


	
	<div class="window wide">
		<div class="windowtitle"><h2>Actions Per Minute and Game History</h2></div>
		<div class="windowcontent padding">
			<style type="text/css">

			</style>
			<!--
				Constructs a single timeline history view that comprises of several timelines.
				
				APM timeline shows the apm of a single player over the whole history in the database.
				It automatically uses a color gradient for the games if it exists.
				
				Game timeline shows each game as a box (a div) extending over the time it was played.
				When the user hovers over a box, it triggers events  that a tooltip can be used to 
				bind to. See .gl-history-timeline-tooltip
				
				Ticks timeline shows the times for timelines				
				
				
				The contents of the container are wrapped to within a single div. When the user zooms
				the view with mouse wheel, the wrapper's width is scaled accordingly and the timelines
				will be updated to extend the whole width of the wrapper. The original container will 
				retain its width, and therefore, the wrapper will extend outside the container (overflow).
				CSS overflow is set to hidden for the container, and therefore, the container provides
				a view to a part of the the wrapper. When the user moves the timelines, only the wrapper
				is moved and everything is updated by the browser automatically.				
				
				TODO: Tablet support
					- Swipe to move
					- Pinch to zoom
					- Zoom buttons				
					- Touch a game to show tooltip
			-->
			<div class="apm-game-history" style="float: left;">
				<div class="player" style="background:url('./elements/mikko-avatar.jpg')"><span class="player">Mikko</span></div>
				<div class="player" style="background:url('./elements/juha-avatar.jpg');"><span class="player">Juha</span></div>
				<div class="player" style="background:url('./elements/sami-avatar.jpg');"><span class="player">Sami</span></div>
				<div class="player" style="background:url('./elements/pavel-avatar.jpg');"><span class="player">Pavel</span></div>
				<div class="player" style="background:url('./elements/tero-avatar.jpg');"><span class="player">Tero</span></div>
				<div class="player" style="background:url('./elements/ville-avatar.jpg');"><span class="player">Ville</span></div>
			</div>
			<div class="gl-history-timeline">
				<div class="gl-history-timeline-game" data-player="mikko"></div>
				<div class="gl-history-timeline-apm" data-apm-max="150" data-player="mikko"></div>

				<div class="gl-history-timeline-game" data-player="juha"></div>
				<div class="gl-history-timeline-apm" data-apm-max="150" data-player="juha"></div>

				<div class="gl-history-timeline-game" data-player="sami"></div>
				<div class="gl-history-timeline-apm" data-apm-max="150" data-player="sami"></div>

				<div class="gl-history-timeline-game" data-player="pavel"></div>
				<div class="gl-history-timeline-apm" data-apm-max="150" data-player="pavel"></div>

				<div class="gl-history-timeline-game" data-player="tero"></div>
				<div class="gl-history-timeline-apm" data-apm-max="150" data-player="tero"></div>

				<div class="gl-history-timeline-game" data-player="ville"></div>
				<div class="gl-history-timeline-apm" data-apm-max="150" data-player="ville"></div>
				<div class="gl-history-timeline-ticks"></div>
			</div>
			<div style="clear: both; ">&nbsp;</div>
			<!--
				A tooltip showing detailed information about a specific game session. When connected to a 
				timeline (automatic) and a user hovers over a game in a timeline, the tooltip is shown and
				its contents is updated. When the mouse leaves the game, it is hidden.
				
				Note: Only the first found tooltip is used and it is connected to all existing 
				gl-history-timeline -elements
				
				data-follow-x attribute defines that the tooltip should follow the horizontal movement of 
				the mouse. The attribute accepts two values: left, right. With left, the CSS property
				left of this class is set to the same value as the user's mouse x. With right, the same
				CSS property is changed, but the value to set is the mouse x-coordingate + tooltip window
				width. 
				
				data-follow-y is the very similar to data-follow-y, but the accepted values are top and bottom.
				
				Note: when data-follow-x or data-follow-y are missing, the corresponding change is not made.
				So if you want for the tooltip to remain in its place, do not define these attributes.
				
				-->
			<div id="debug"></div>
			<div class="gl-history-timeline-tooltip" data-follow-x="left" data-follow-y="top" style="pointer-events: none;">											
				<div class="gl-game-name">gamename</div>
				<span>Play time:</span>				
				<div class="gl-game-played">played</div>		
				<!-- div class="gl-game-id">gameid</div> -->
				<span>Total Actions per session:</span>				
				<div class="gl-apm-total">total clicks</div>
				<span>Average Action per session:</span>				
				<div class="gl-apm-avg">avg apm</div>		
				<!--<div class="gl-player">player</div>-->						
				<!-- <div class="gl-player-class">class of player</div>-->
				<span>Focus time:</span>						
				<div class="gl-history-timeline-time"></div>
				<div class="gl-game-logo">logo</div>		
				<!-- <div class="gl-game-logo-bg">logobg</div>-->		
			</div>
		</div>
			<p style="clear: both";>Shows zoomable histogram for 'actions per minute' counter with games played. Hower your mouse above graph to see time and game data, middle mouse to zoom in and zoom out.</p>
	</div>


<!-- Gamelogger history ends -->

</body>
</html>