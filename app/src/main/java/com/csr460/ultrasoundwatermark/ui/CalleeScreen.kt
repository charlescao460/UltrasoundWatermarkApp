package com.csr460.ultrasoundwatermark.ui

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp

@Composable
fun CalleeScreen(viewModel: MainViewModel) {
    val state by viewModel.calleeState.collectAsState()

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text("My IP Addresses", style = MaterialTheme.typography.headlineSmall)
        Spacer(modifier = Modifier.height(8.dp))
        Text("IPv4: ${state.ipv4Address}")
        Text("IPv6: ${state.ipv6Address}")
        Spacer(modifier = Modifier.height(16.dp))
        Button(onClick = {
            if (state.isListening) {
                viewModel.stopCallee()
            } else {
                viewModel.startCallee()
            }
        }) {
            Text(if (state.isListening) "Stop Listening" else "Start Listening")
        }
        Spacer(modifier = Modifier.height(16.dp))
        if (state.isListening) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                CircularProgressIndicator()
                Spacer(modifier = Modifier.width(8.dp))
                Text("Listening...")
            }
        }
        Spacer(modifier = Modifier.height(16.dp))
        Text("Instantaneous: ${state.instantaneousProbability}")
        Text("Average: ${state.averageProbability}")
        Spacer(modifier = Modifier.height(8.dp))
        val isWatermarked = state.averageProbability > state.probabilityThreshold
        Text(
            text = if (isWatermarked) "Watermarked" else "Not Watermarked",
            color = if (isWatermarked) Color.Green else Color.Red,
            style = MaterialTheme.typography.headlineMedium
        )
    }
}