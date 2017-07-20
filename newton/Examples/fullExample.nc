dimensionTypeNames {
    time = "s";
    distance = "m";
    displacement = "m";
    mass = "g";
    angle = "rad";
}

vectorScalarPairs {
    displacement = distance;
    velocity = speed;
    acceleration = scalar_acceleration;
    force = scalar_force;
}

law {
    velocity = displacement / time;
    acceleration = velocity / time;
    force = mass * acceleration;

    speed = distance / time;
    scalar_acceleration = speed / time;
    scalar_force = mass * scalar_acceleration;

    work = dot(force, displacement);
    torque = cross(force, displacement);
}

dimensionAliases {
    work = "J";
    force = "N";
}

vectorIntegrals {
    [displacement, velocity, acceleration];
}

scalarIntegrals {
    [distance, speed, scalar_acceleration];
}

