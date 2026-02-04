package com.csr460.ultrasoundwatermark.ui

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

@Composable
fun CallerScreen(viewModel: MainViewModel) {
    val state by viewModel.callerState.collectAsState()

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        OutlinedTextField(
            value = state.hostname,
            onValueChange = { viewModel.onHostNameChange(it) },
            label = { Text("Hostname") },
            modifier = Modifier.fillMaxWidth()
        )
        Spacer(modifier = Modifier.height(16.dp))
        Button(onClick = {
            if (state.isCalling) {
                viewModel.stopCaller()
            } else {
                viewModel.startCaller()
            }
        }) {
            Text(if (state.isCalling) "Stop Call" else "Start Call")
        }
        Spacer(modifier = Modifier.height(16.dp))
        if (state.isCalling) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                CircularProgressIndicator()
                Spacer(modifier = Modifier.width(8.dp))
                Text("Calling...")
            }
        }
    }
}