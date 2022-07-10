<script>
	import Disk from './Disk.svelte'
	import Fa from 'svelte-fa'
	import { faPlay, faStop, faPause } from '@fortawesome/free-solid-svg-icons'

	const disk0 = {
		name: 'Data 0',
		blocks: 104,
		used: 30
	}

	const disk1 = {
		name: 'Data 1',
		blocks: 104,
		used: 34
	}

	const disk2 = {
		name: 'Data 2',
		blocks: 104,
		used: 31
	}

	const disk3 = {
		name: 'Data 3',
		blocks: 104,
		used: 40
	}

	const parity0 = {
		name: 'Parity P',
		blocks: 104,
		used: 16
	}

	const parity1 = {
		name: 'Parity Q',
		blocks: 104,
		used: 16
	}
</script>

<main>
	<div class="progress">
		<div class="progress-perc">16%</div>
	</div>
	<h1>RAID6</h1>
	<h3>szfv-offline</h3>
	<p>initializing</p>
	<Disk {...disk0}/> <Disk {...disk1}/> <Disk {...disk2}/> <Disk {...disk3}/> <Disk {...parity0}/> <Disk {...parity1}/>
	<div class="button-panel">
		<div class="pbutton inactive">
			<Fa icon={faPlay} />
		</div>
		<div class="pbutton">
			<Fa icon={faPause} />
		</div>
		<div class="pbutton">
			<Fa icon={faStop} />
		</div>
	</div>
	<div class="info">
		<span>
			<p>Disk capacity:  12 TB</p>
			<p>Total capacity: 48 TB</p>
			<p>Used capacity:  15,950 GB</p>
		</span>
		<span>
			<p>Status: Good, initializing</p>
			<p>Last checked: never</p>
			<p>Current op. ETA: 1 day 3 hours</p>
		</span>
		<span>
			<p>Algorithm: Reed-Solomon</p>
			<p>Failures: none</p>
			<p>Failure mode: none</p>
		</span>
		<span style="border: none">
			<p>CPU:       25%</p>
			<p>HDD read:  1054MB/s</p>
			<p>HDD write: 268MB/s</p>
		</span>
	</div>
</main>

<style>
	main {
		text-align: center;
		padding: 1em;
		max-width: 240px;
		margin: 0 auto;
	}

	h1 {
		color: rgb(128, 0, 128);
		text-transform: uppercase;
		font-size: 4em;
		font-weight: 100;
		margin: 0 0 15px 0;
	}

	.progress {
		position: absolute;
		width: 977px;
		height: 218px;
		background-color: transparent;
		z-index: 10;
		border-bottom: 2px solid rgb(128, 0, 128);
	}

	.progress-perc {
		position: absolute;
		bottom: 0;
		right: 0;
		color: rgb(128, 0, 128);
	}

	.pbutton {
		display: inline-block;
		font-size: 2em;
		color: rgba(192, 32, 192, 1);
		margin: 0 10px 0 10px;
	}

	.inactive {
		color: rgb(128, 0, 128);
	}
	
	.info {
		text-align: left;
		padding-top: 10px;
	}

	.info span {
		display:inline-block;
		margin-left: 21px;
		border-right: 1px solid gray;
		padding-right: 21px;
	}

	@media (min-width: 640px) {
		main {
			max-width: none;
		}
	}
</style>