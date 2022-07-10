<script>
	export let name
    export let blocks
    export let used
</script>

<div class="main">
    <div class="box">
        {#each [...Array(blocks).keys()] as _, i}
            <div class="block" style="background-color: {i < used ? 'green' : 'lightgray'}"/>
        {/each}
    </div>
    <p class="lbl">{name}</p>
</div>

<style>
    .main {
        display: inline-block;
        width: 140px;
        margin: 0 6px 0 6px;
    }
    
    .box {
        width: 100%;
        height: 220px;
        border: 1px solid;
        border-color: black;
        padding: 0 0 2px 2px;
    }

    .block {
        float: left;
        position: relative;
        width: 6%;
        padding-bottom: 6%;
        margin: 1.66%;
        border: 2px solid gray;
    }
</style>